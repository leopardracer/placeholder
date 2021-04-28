//---------------------------------------------------------------------------//
// Copyright (c) 2018-2021 Mikhail Komarov <nemo@nil.foundation>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the Server Side Public License, version 1,
// as published by the author.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Server Side Public License for more details.
//
// You should have received a copy of the Server Side Public License
// along with this program. If not, see
// <https://github.com/NilFoundation/dbms/blob/master/LICENSE_1_0.txt>.
//---------------------------------------------------------------------------//

#include <nil/actor/core/thread.hh>
#include <nil/actor/core/posix.hh>
#include <nil/actor/core/reactor.hh>

#include <ucontext.h>
#include <algorithm>

#ifdef ACTOR_HAS_VALGRIND
#include <valgrind/valgrind.h>
#endif

/// \cond internal

namespace nil {
    namespace actor {

        thread_local jmp_buf_link g_unthreaded_context;
        thread_local jmp_buf_link *g_current_context;

#ifdef ACTOR_ASAN_ENABLED

        namespace {

#ifdef ACTOR_HAVE_ASAN_FIBER_SUPPORT
            // ASan provides two functions as a means of informing it that user context
            // switch has happened. First __sanitizer_start_switch_fiber() needs to be
            // called with a place to store the fake stack pointer and the new stack
            // information as arguments. Then, ucontext switch may be performed after which
            // __sanitizer_finish_switch_fiber() needs to be called with a pointer to the
            // current context fake stack and a place to store stack information of the
            // previous ucontext.

            extern "C" {
            void __sanitizer_start_switch_fiber(void **fake_stack_save, const void *stack_bottom, size_t stack_size);
            void __sanitizer_finish_switch_fiber(void *fake_stack_save, const void **stack_bottom_old,
                                                 size_t *stack_size_old);
            }
#else
            static inline void __sanitizer_start_switch_fiber(...) {
            }
            static inline void __sanitizer_finish_switch_fiber(...) {
            }
#endif

            thread_local jmp_buf_link *g_previous_context;

        }    // namespace

        void jmp_buf_link::initial_switch_in(ucontext_t *initial_context, const void *stack_bottom, size_t stack_size) {
            auto prev = std::exchange(g_current_context, this);
            link = prev;
            g_previous_context = prev;
            __sanitizer_start_switch_fiber(&prev->fake_stack, stack_bottom, stack_size);
            swapcontext(&prev->context, initial_context);
            __sanitizer_finish_switch_fiber(g_current_context->fake_stack, &g_previous_context->stack_bottom,
                                            &g_previous_context->stack_size);
        }

        void jmp_buf_link::switch_in() {
            auto prev = std::exchange(g_current_context, this);
            link = prev;
            g_previous_context = prev;
            __sanitizer_start_switch_fiber(&prev->fake_stack, stack_bottom, stack_size);
            swapcontext(&prev->context, &context);
            __sanitizer_finish_switch_fiber(g_current_context->fake_stack, &g_previous_context->stack_bottom,
                                            &g_previous_context->stack_size);
        }

        void jmp_buf_link::switch_out() {
            g_current_context = link;
            g_previous_context = this;
            __sanitizer_start_switch_fiber(&fake_stack, g_current_context->stack_bottom, g_current_context->stack_size);
            swapcontext(&context, &g_current_context->context);
            __sanitizer_finish_switch_fiber(g_current_context->fake_stack, &g_previous_context->stack_bottom,
                                            &g_previous_context->stack_size);
        }

        void jmp_buf_link::initial_switch_in_completed() {
            // This is a new thread and it doesn't have the fake stack yet. ASan will
            // create it lazily, for now just pass nullptr.
            __sanitizer_finish_switch_fiber(nullptr, &g_previous_context->stack_bottom,
                                            &g_previous_context->stack_size);
        }

        void jmp_buf_link::final_switch_out() {
            g_current_context = link;
            g_previous_context = this;
            // Since the thread is about to die we pass nullptr as fake_stack_save argument
            // so that ASan knows it can destroy the fake stack if it exists.
            __sanitizer_start_switch_fiber(nullptr, g_current_context->stack_bottom, g_current_context->stack_size);
            setcontext(&g_current_context->context);
        }

#else

        inline void jmp_buf_link::initial_switch_in(ucontext_t *initial_context, const void *, size_t) {
            auto prev = std::exchange(g_current_context, this);
            link = prev;
            if (setjmp(prev->jmpbuf) == 0) {
                setcontext(initial_context);
            }
        }

        inline void jmp_buf_link::switch_in() {
            auto prev = std::exchange(g_current_context, this);
            link = prev;
            if (setjmp(prev->jmpbuf) == 0) {
                longjmp(jmpbuf, 1);
            }
        }

        inline void jmp_buf_link::switch_out() {
            g_current_context = link;
            if (setjmp(jmpbuf) == 0) {
                longjmp(g_current_context->jmpbuf, 1);
            }
        }

        inline void jmp_buf_link::initial_switch_in_completed() {
        }

        inline void jmp_buf_link::final_switch_out() {
            g_current_context = link;
            longjmp(g_current_context->jmpbuf, 1);
        }

#endif

// Both asan and optimizations can increase the stack used by a
// function. When both are used, we need more than 128 KiB.
#if defined(ACTOR_ASAN_ENABLED)
        static constexpr size_t base_stack_size = 256 * 1024;
#else
        static constexpr size_t base_stack_size = 128 * 1024;
#endif

        static size_t get_stack_size(thread_attributes attr) {
#if defined(__OPTIMIZE__) && defined(ACTOR_ASAN_ENABLED)
            return std::max(base_stack_size, attr.stack_size);
#else
            return attr.stack_size ? attr.stack_size : base_stack_size;
#endif
        }

        thread_context::thread_context(thread_attributes attr, noncopyable_function<void()> func) :
            task(attr.sched_group.value_or(current_scheduling_group())), _stack(make_stack(get_stack_size(attr))),
            _func(std::move(func)) {
            setup(get_stack_size(attr));
            _all_threads.push_front(*this);
        }

        thread_context::~thread_context() {
#ifdef ACTOR_THREAD_STACK_GUARDS
            auto mp_result = mprotect(_stack.get(), getpagesize(), PROT_READ | PROT_WRITE);
            assert(mp_result == 0);
#endif
            _all_threads.erase(_all_threads.iterator_to(*this));
        }

        thread_context::stack_deleter::stack_deleter(int valgrind_id) : valgrind_id(valgrind_id) {
        }

        thread_context::stack_holder thread_context::make_stack(size_t stack_size) {
#ifdef ACTOR_THREAD_STACK_GUARDS
            size_t page_size = getpagesize();
            size_t alignment = page_size;
#else
            size_t alignment = 16;    // ABI requirement on x86_64
#endif
            void *mem = ::aligned_alloc(alignment, stack_size);
            if (mem == nullptr) {
                throw std::bad_alloc();
            }

            int valgrind_id = VALGRIND_STACK_REGISTER(mem, reinterpret_cast<char *>(mem) + stack_size);
            auto stack = stack_holder(new (mem) char[stack_size], stack_deleter(valgrind_id));
#ifdef ACTOR_ASAN_ENABLED
            // Avoid ASAN false positive due to garbage on stack
            std::fill_n(stack.get(), stack_size, 0);
#endif

#ifdef ACTOR_THREAD_STACK_GUARDS
            auto mp_status = mprotect(stack.get(), page_size, PROT_READ);
            throw_system_error_on(mp_status != 0, "mprotect");
#endif

            return stack;
        }

        void thread_context::stack_deleter::operator()(char *ptr) const noexcept {
            VALGRIND_STACK_DEREGISTER(valgrind_id);
            free(ptr);
        }

        void thread_context::setup(size_t stack_size) {
            // use setcontext() for the initial jump, as it allows us
            // to set up a stack, but continue with longjmp() as it's
            // much faster.
            ucontext_t initial_context;
            auto q = uint64_t(reinterpret_cast<uintptr_t>(this));
            auto main = reinterpret_cast<void (*)()>(&thread_context::s_main);
            auto r = getcontext(&initial_context);
            throw_system_error_on(r == -1);
            initial_context.uc_stack.ss_sp = _stack.get();
            initial_context.uc_stack.ss_size = stack_size;
            initial_context.uc_link = nullptr;
            makecontext(&initial_context, main, 2, int(q), int(q >> 32));
            _context.thread = this;
            _context.initial_switch_in(&initial_context, _stack.get(), stack_size);
        }

        void thread_context::switch_in() {
            local_engine->_current_task =
                nullptr;    // thread_wake_task is on the stack and will be invalid when we resume
            _context.switch_in();
        }

        void thread_context::switch_out() {
            _context.switch_out();
        }

        bool thread_context::should_yield() const {
            return need_preempt();
        }

        thread_local thread_context::all_thread_list thread_context::_all_threads;

        void thread_context::run_and_dispose() noexcept {
            switch_in();
        }

        void thread_context::yield() {
            schedule(this);
            switch_out();
        }

        void thread_context::reschedule() {
            schedule(this);
        }

        void thread_context::s_main(int lo, int hi) {
            uintptr_t q = uint64_t(uint32_t(lo)) | uint64_t(hi) << 32;
            reinterpret_cast<thread_context *>(q)->main();
        }

        void thread_context::main() {
#if BOOST_OS_LINUX && defined(__x86_64__)
            // There is no caller of main() in this context. We need to annotate this frame like this so that
            // unwinders don't try to trace back past this frame.
            // See https://github.com/scylladb/scylla/issues/1909.
            asm(".cfi_undefined rip");
#elif BOOST_OS_LINUX && defined(__PPC__)
            asm(".cfi_undefined lr");
#elif BOOST_OS_LINUX && defined(__aarch64__)
            asm(".cfi_undefined x30");
#elif BOOST_OS_LINUX
#warning "Backtracing from seastar threads may be broken"
#endif
            _context.initial_switch_in_completed();
            if (group() != current_scheduling_group()) {
                yield();
            }
            try {
                _func();
                _done.set_value();
            } catch (...) {
                _done.set_exception(std::current_exception());
            }

            _context.final_switch_out();
        }

        namespace thread_impl {

            void yield() {
                g_current_context->thread->yield();
            }

            void switch_in(thread_context *to) {
                to->switch_in();
            }

            void switch_out(thread_context *from) {
                from->switch_out();
            }

            void init() {
                g_unthreaded_context.link = nullptr;
                g_unthreaded_context.thread = nullptr;
                g_current_context = &g_unthreaded_context;
            }

            scheduling_group sched_group(const thread_context *thread) {
                return thread->group();
            }

        }    // namespace thread_impl

        void thread::yield() {
            thread_impl::get()->yield();
        }

        bool thread::should_yield() {
            return thread_impl::get()->should_yield();
        }

        void thread::maybe_yield() {
            auto tctx = thread_impl::get();
            if (tctx->should_yield()) {
                tctx->yield();
            }
        }
    }    // namespace actor
}    // namespace nil

/// \endcond
