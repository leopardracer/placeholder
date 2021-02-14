//---------------------------------------------------------------------------//
// Copyright (c) 2018-2021 Mikhail Komarov <nemo@nil.foundation>
//
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//---------------------------------------------------------------------------//

#include <algorithm>
#include <iostream>
#include <numeric>
#include <nil/actor/core/fstream.hh>
#include <nil/actor/core/smp.hh>
#include <nil/actor/core/shared_ptr.hh>
#include <nil/actor/core/app-template.hh>
#include <nil/actor/core/do_with.hh>
#include <nil/actor/core/core.hh>
#include <nil/actor/core/semaphore.hh>
#include <nil/actor/testing/test_case.hh>
#include <nil/actor/testing/test_runner.hh>
#include <nil/actor/core/thread.hh>
#include <nil/actor/core/print.hh>
#include <nil/actor/detail/defer.hh>
#include <nil/actor/detail/tmp_file.hh>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/algorithm/cxx11/any_of.hpp>
#include "mock_file.hh"
#include <boost/range/irange.hpp>

using namespace nil::actor;
namespace fs = std::filesystem;

struct writer {
    output_stream<char> out;
    static future<shared_ptr<writer>> make(file f) {
        return api_v3::and_newer::make_file_output_stream(std::move(f)).then([](output_stream<char> &&os) {
            return make_shared<writer>(writer {std::move(os)});
        });
    }
};

struct reader {
    input_stream<char> in;
    reader(file f) : in(make_file_input_stream(std::move(f))) {
    }
    reader(file f, file_input_stream_options options) : in(make_file_input_stream(std::move(f), std::move(options))) {
    }
};

ACTOR_TEST_CASE(test_fstream) {
    return tmp_dir::do_with([](tmp_dir &t) {
        auto filename = (t.get_path() / "testfile.tmp").native();
        return open_file_dma(filename, open_flags::rw | open_flags::create | open_flags::truncate)
            .then([filename](file f) {
                return writer::make(std::move(f)).then([filename](shared_ptr<writer> w) {
                    auto buf = static_cast<char *>(::malloc(4096));
                    memset(buf, 0, 4096);
                    buf[0] = '[';
                    buf[1] = 'A';
                    buf[4095] = ']';
                    return w->out.write(buf, 4096)
                        .then([buf, w] {
                            ::free(buf);
                            return make_ready_future<>();
                        })
                        .then([w] {
                            auto buf = static_cast<char *>(::malloc(8192));
                            memset(buf, 0, 8192);
                            buf[0] = '[';
                            buf[1] = 'B';
                            buf[8191] = ']';
                            return w->out.write(buf, 8192).then([buf, w] {
                                ::free(buf);
                                return w->out.close().then([w] {});
                            });
                        })
                        .then([filename] { return open_file_dma(filename, open_flags::ro); })
                        .then([](file f) {
                            /*  file content after running the above:
                             * 00000000  5b 41 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |[A..............|
                             * 00000010  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
                             * *
                             * 00000ff0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 5d  |...............]|
                             * 00001000  5b 42 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |[B..............|
                             * 00001010  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
                             * *
                             * 00002ff0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 5d  |...............]|
                             * 00003000
                             */
                            auto r = make_shared<reader>(std::move(f));
                            return r->in.read_exactly(4096 + 8192)
                                .then([r](temporary_buffer<char> buf) {
                                    auto p = buf.get();
                                    BOOST_REQUIRE(p[0] == '[' && p[1] == 'A' && p[4095] == ']');
                                    BOOST_REQUIRE(p[4096] == '[' && p[4096 + 1] == 'B' && p[4096 + 8191] == ']');
                                    return make_ready_future<>();
                                })
                                .then([r] { return r->in.close(); })
                                .finally([r] {});
                        });
                });
            });
    });
}

ACTOR_TEST_CASE(test_consume_skip_bytes) {
    return tmp_dir::do_with_thread([](tmp_dir &t) {
        auto filename = (t.get_path() / "testfile.tmp").native();
        auto f = open_file_dma(filename, open_flags::rw | open_flags::create | open_flags::truncate).get0();
        auto w = writer::make(std::move(f)).get0();
        auto write_block = [w](char c, size_t size) {
            std::vector<char> vec(size, c);
            w->out.write(&vec.front(), vec.size()).get();
        };
        write_block('a', 8192);
        write_block('b', 8192);
        w->out.close().get();
        /*  file content after running the above:
         * 00000000  61 61 61 61 61 61 61 61  61 61 61 61 61 61 61 61  |aaaaaaaaaaaaaaaa|
         * *
         * 00002000  62 62 62 62 62 62 62 62  62 62 62 62 62 62 62 62  |bbbbbbbbbbbbbbbb|
         * *
         * 00004000
         */
        f = open_file_dma(filename, open_flags::ro).get0();
        auto r = make_lw_shared<reader>(std::move(f), file_input_stream_options {512});
        struct consumer {
            uint64_t _count = 0;
            using consumption_result_type = typename input_stream<char>::consumption_result_type;
            using stop_consuming_type = typename consumption_result_type::stop_consuming_type;
            using tmp_buf = stop_consuming_type::tmp_buf;

            /*
             * Consumer reads the file as follows:
             *  - first 8000 bytes are read in 512-byte chunks and checked
             *  - next 2000 bytes are skipped (jumping over both read buffer size and DMA block)
             *  - the remaining 6384 bytes are read and checked
             */
            future<consumption_result_type> operator()(tmp_buf buf) {
                if (_count < 8000) {
                    auto delta = std::min(buf.size(), 8000 - _count);
                    for (auto c : buf.share(0, delta)) {
                        BOOST_REQUIRE_EQUAL(c, 'a');
                    }
                    buf.trim_front(delta);
                    _count += delta;

                    if (_count == 8000) {
                        return make_ready_future<consumption_result_type>(skip_bytes {2000 - buf.size()});
                    } else {
                        assert(buf.empty());
                        return make_ready_future<consumption_result_type>(continue_consuming {});
                    }
                    return make_ready_future<consumption_result_type>(continue_consuming {});
                } else {
                    for (auto c : buf) {
                        BOOST_REQUIRE_EQUAL(c, 'b');
                    }
                    _count += buf.size();
                    if (_count < 14384) {
                        return make_ready_future<consumption_result_type>(continue_consuming {});
                    } else if (_count > 14384) {
                        BOOST_FAIL("Read more than expected");
                    }
                    return make_ready_future<consumption_result_type>(stop_consuming_type({}));
                }
            }
        };
        r->in.consume(consumer {}).get();
        r->in.close().get();
    });
}

ACTOR_TEST_CASE(test_fstream_unaligned) {
    return tmp_dir::do_with([](tmp_dir &t) {
        auto filename = (t.get_path() / "testfile.tmp").native();
        return open_file_dma(filename, open_flags::rw | open_flags::create | open_flags::truncate)
            .then([filename](file f) {
                return writer::make(std::move(f)).then([filename](shared_ptr<writer> w) {
                    auto buf = static_cast<char *>(::malloc(40));
                    memset(buf, 0, 40);
                    buf[0] = '[';
                    buf[1] = 'A';
                    buf[39] = ']';
                    return w->out.write(buf, 40)
                        .then([buf, w] {
                            ::free(buf);
                            return w->out.close().then([w] {});
                        })
                        .then([filename] { return open_file_dma(filename, open_flags::ro); })
                        .then([](file f) {
                            return do_with(std::move(f), [](file &f) {
                                return f.size().then([](size_t size) {
                                    // assert that file was indeed truncated to the amount of bytes written.
                                    BOOST_REQUIRE(size == 40);
                                    return make_ready_future<>();
                                });
                            });
                        })
                        .then([filename] { return open_file_dma(filename, open_flags::ro); })
                        .then([](file f) {
                            auto r = make_shared<reader>(std::move(f));
                            return r->in.read_exactly(40)
                                .then([r](temporary_buffer<char> buf) {
                                    auto p = buf.get();
                                    BOOST_REQUIRE(p[0] == '[' && p[1] == 'A' && p[39] == ']');
                                    return make_ready_future<>();
                                })
                                .then([r] { return r->in.close(); })
                                .finally([r] {});
                        });
                });
            });
    });
}

future<> test_consume_until_end(uint64_t size) {
    return tmp_dir::do_with([size](tmp_dir &t) {
        auto filename = (t.get_path() / "testfile.tmp").native();
        return open_file_dma(filename, open_flags::rw | open_flags::create | open_flags::truncate).then([size](file f) {
            return api_v3::and_newer::make_file_output_stream(f)
                .then([size](output_stream<char> &&os) {
                    return do_with(std::move(os), [size](output_stream<char> &out) {
                        std::vector<char> buf(size);
                        std::iota(buf.begin(), buf.end(), 0);
                        return out.write(buf.data(), buf.size()).then([&out] { return out.flush(); });
                    });
                })
                .then([f] { return f.size(); })
                .then([size, f](size_t real_size) { BOOST_REQUIRE_EQUAL(size, real_size); })
                .then([size, f] {
                    auto consumer =
                        [offset = uint64_t(0),
                         size](temporary_buffer<char> buf) mutable -> future<input_stream<char>::unconsumed_remainder> {
                        if (!buf) {
                            return make_ready_future<input_stream<char>::unconsumed_remainder>(
                                temporary_buffer<char>());
                        }
                        BOOST_REQUIRE(offset + buf.size() <= size);
                        std::vector<char> expected(buf.size());
                        std::iota(expected.begin(), expected.end(), offset);
                        offset += buf.size();
                        BOOST_REQUIRE(std::equal(buf.begin(), buf.end(), expected.begin()));
                        return make_ready_future<input_stream<char>::unconsumed_remainder>(std::nullopt);
                    };
                    return do_with(make_file_input_stream(f), std::move(consumer),
                                   [](input_stream<char> &in, auto &consumer) {
                                       return in.consume(consumer).then([&in] { return in.close(); });
                                   });
                })
                .finally([f]() mutable { return f.close(); });
        });
    });
}

ACTOR_TEST_CASE(test_consume_aligned_file) {
    return test_consume_until_end(4096);
}

ACTOR_TEST_CASE(test_consume_empty_file) {
    return test_consume_until_end(0);
}

ACTOR_TEST_CASE(test_consume_unaligned_file) {
    return test_consume_until_end(1);
}

ACTOR_TEST_CASE(test_consume_unaligned_file_large) {
    return test_consume_until_end((1 << 20) + 1);
}

ACTOR_TEST_CASE(test_input_stream_esp_around_eof) {
    return tmp_dir::do_with_thread([](tmp_dir &t) {
        auto flen = uint64_t(5341);
        auto rdist = std::uniform_int_distribution<char>();
        auto reng = testing::local_random_engine;
        auto data = boost::copy_range<std::vector<uint8_t>>(
            boost::irange<uint64_t>(0, flen) | boost::adaptors::transformed([&](int x) { return rdist(reng); }));
        auto filename = (t.get_path() / "testfile.tmp").native();
        auto f = open_file_dma(filename, open_flags::rw | open_flags::create | open_flags::truncate).get0();
        auto out = api_v3::and_newer::make_file_output_stream(f).get0();
        out.write(reinterpret_cast<const char *>(data.data()), data.size()).get();
        out.flush().get();
        // out.close().get();  // FIXME: closes underlying stream:?!
        struct range {
            uint64_t start;
            uint64_t end;
        };
        auto ranges = std::vector<range> {{
            range {0, flen},
            range {0, flen * 2},
            range {0, flen + 1},
            range {0, flen - 1},
            range {0, 1},
            range {1, 2},
            range {flen - 1, flen},
            range {flen - 1, flen + 1},
            range {flen, flen + 1},
            range {flen + 1, flen + 2},
            range {1023, flen - 1},
            range {1023, flen},
            range {1023, flen + 2},
            range {8193, 8194},
            range {1023, 1025},
            range {1023, 1024},
            range {1024, 1025},
            range {1023, 4097},
        }};
        auto opt = file_input_stream_options();
        opt.buffer_size = 512;
        for (auto &&r : ranges) {
            auto start = r.start;
            auto end = r.end;
            auto len = end - start;
            auto in = make_file_input_stream(f, start, len, opt);
            std::vector<uint8_t> readback;
            auto more = true;
            while (more) {
                auto rdata = in.read().get0();
                for (size_t i = 0; i < rdata.size(); ++i) {
                    readback.push_back(rdata.get()[i]);
                }
                more = !rdata.empty();
            }
            // in.close().get();
            auto xlen = std::min(end, flen) - std::min(flen, start);
            if (xlen != readback.size()) {
                BOOST_FAIL(format("Expected {:d} bytes but got {:d}, start={:d}, end={:d}", xlen, readback.size(),
                                  start, end));
            }
            BOOST_REQUIRE(std::equal(readback.begin(), readback.end(), data.begin() + std::min(start, flen)));
        }
        f.close().get();
    });
}

#if ACTOR_API_LEVEL >= 3
ACTOR_TEST_CASE(without_api_prefix) {
    return tmp_dir::do_with_thread([](tmp_dir &t) {
        auto filename = (t.get_path() / "testfile.tmp").native();
        auto f = open_file_dma(filename, open_flags::rw | open_flags::create | open_flags::truncate).get0();
        output_stream<char> out = make_file_output_stream(f).get0();
        out.close().get();
    });
}
#endif

ACTOR_TEST_CASE(file_handle_test) {
    return tmp_dir::do_with_thread([](tmp_dir &t) {
        auto filename = (t.get_path() / "testfile.tmp").native();
        auto f = open_file_dma(filename, open_flags::create | open_flags::truncate | open_flags::rw).get0();
        auto buf = static_cast<char *>(aligned_alloc(4096, 4096));
        auto del = defer([&] { ::free(buf); });
        for (unsigned i = 0; i < 4096; ++i) {
            buf[i] = i;
        }
        f.dma_write(0, buf, 4096).get();
        auto bad = std::vector<unsigned>(
            smp::count);    // std::vector<bool> is special and unsuitable because it uses bitfields
        smp::invoke_on_all([fh = f.dup(), &bad] {
            return nil::actor::async([fh, &bad] {
                auto f = fh.to_file();
                auto buf = static_cast<char *>(aligned_alloc(4096, 4096));
                auto del = defer([&] { ::free(buf); });
                f.dma_read(0, buf, 4096).get();
                for (unsigned i = 0; i < 4096; ++i) {
                    bad[this_shard_id()] |= buf[i] != char(i);
                }
            });
        }).get();
        BOOST_REQUIRE(!boost::algorithm::any_of_equal(bad, 1u));
        f.close().get();
    });
}

ACTOR_TEST_CASE(test_fstream_slow_start) {
    return nil::actor::async([] {
        static constexpr size_t file_size = 128 * 1024 * 1024;
        static constexpr size_t buffer_size = 260 * 1024;
        static constexpr size_t read_ahead = 1;

        auto mock_file = make_shared<mock_read_only_file>(file_size);

        auto history = make_lw_shared<file_input_stream_history>();

        file_input_stream_options options {};
        options.buffer_size = buffer_size;
        options.read_ahead = read_ahead;
        options.dynamic_adjustments = history;

        static constexpr size_t requests_at_slow_start = 2;                 // 1 request + 1 read-ahead
        static constexpr size_t requests_at_full_speed = read_ahead + 1;    // 1 request + read_ahead

        std::optional<size_t> initial_read_size;

        auto read_whole_file_with_slow_start = [&](auto fstr) {
            uint64_t total_read = 0;
            size_t previous_buffer_length = 0;

            // We don't want to assume too much about fstream internals, but with
            // no history we should start with a buffer sizes somewhere in
            // (0, buffer_size) range.
            mock_file->set_read_size_verifier([&](size_t length) {
                BOOST_CHECK_LE(length, initial_read_size.value_or(buffer_size - 1));
                BOOST_CHECK_GE(length, initial_read_size.value_or(1));
                previous_buffer_length = length;
                if (!initial_read_size) {
                    initial_read_size = length;
                }
            });

            // Slow start phase
            while (true) {
                // We should leave slow start before reading the whole file.
                BOOST_CHECK_LT(total_read, file_size);

                mock_file->set_allowed_read_requests(requests_at_slow_start);
                auto buf = fstr.read().get0();
                BOOST_CHECK_GT(buf.size(), 0u);

                mock_file->set_read_size_verifier([&](size_t length) {
                    // There is no reason to reduce buffer size.
                    BOOST_CHECK_LE(length, std::min(previous_buffer_length * 2, buffer_size));
                    BOOST_CHECK_GE(length, previous_buffer_length);
                    previous_buffer_length = length;
                });

                BOOST_TEST_MESSAGE(format("Size {:d}", buf.size()));
                total_read += buf.size();
                if (buf.size() == buffer_size) {
                    BOOST_TEST_MESSAGE("Leaving slow start phase.");
                    break;
                }
            }

            // Reading at full speed now
            mock_file->set_expected_read_size(buffer_size);
            while (total_read != file_size) {
                mock_file->set_allowed_read_requests(requests_at_full_speed);
                auto buf = fstr.read().get0();
                total_read += buf.size();
            }

            mock_file->set_allowed_read_requests(requests_at_full_speed);
            auto buf = fstr.read().get0();
            BOOST_CHECK_EQUAL(buf.size(), 0u);
            assert(buf.size() == 0);
        };

        auto read_while_file_at_full_speed = [&](auto fstr) {
            uint64_t total_read = 0;

            mock_file->set_expected_read_size(buffer_size);
            while (total_read != file_size) {
                mock_file->set_allowed_read_requests(requests_at_full_speed);
                auto buf = fstr.read().get0();
                total_read += buf.size();
            }

            mock_file->set_allowed_read_requests(requests_at_full_speed);
            auto buf = fstr.read().get0();
            BOOST_CHECK_EQUAL(buf.size(), 0u);
        };

        auto read_and_skip_a_lot = [&](auto fstr) {
            uint64_t total_read = 0;
            size_t previous_buffer_size = buffer_size;

            mock_file->set_allowed_read_requests(std::numeric_limits<size_t>::max());
            mock_file->set_read_size_verifier([&](size_t length) {
                // There is no reason to reduce buffer size.
                BOOST_CHECK_LE(length, previous_buffer_size);
                BOOST_CHECK_GE(length, initial_read_size.value_or(1));
                previous_buffer_size = length;
            });
            while (total_read != file_size) {
                auto buf = fstr.read().get0();
                total_read += buf.size();

                buf = fstr.read().get0();
                total_read += buf.size();

                auto skip_by = std::min(file_size - total_read, buffer_size * 2);
                fstr.skip(skip_by).get();
                total_read += skip_by;
            }

            // We should be back at slow start at this stage.
            BOOST_CHECK_LT(previous_buffer_size, buffer_size);
            if (initial_read_size) {
                BOOST_CHECK_EQUAL(previous_buffer_size, *initial_read_size);
            }

            mock_file->set_allowed_read_requests(requests_at_full_speed);
            auto buf = fstr.read().get0();
            BOOST_CHECK_EQUAL(buf.size(), 0u);
        };

        auto make_fstream = [&] {
            struct fstream_wrapper {
                input_stream<char> s;
                explicit fstream_wrapper(input_stream<char> &&s) : s(std::move(s)) {
                }
                fstream_wrapper(fstream_wrapper &&) = default;
                fstream_wrapper &operator=(fstream_wrapper &&) = default;
                future<temporary_buffer<char>> read() {
                    return s.read();
                }
                future<> skip(uint64_t n) {
                    return s.skip(n);
                }
                ~fstream_wrapper() {
                    s.close().get();
                }
            };
            return fstream_wrapper(make_file_input_stream(file(mock_file), 0, file_size, options));
        };

        BOOST_TEST_MESSAGE("Reading file, no history, expectiong a slow start");
        read_whole_file_with_slow_start(make_fstream());
        BOOST_TEST_MESSAGE("Reading file again, everything good so far, read at full speed");
        read_while_file_at_full_speed(make_fstream());
        BOOST_TEST_MESSAGE("Reading and skipping a lot");
        read_and_skip_a_lot(make_fstream());
        BOOST_TEST_MESSAGE("Reading file, bad history, we are back at slow start...");
        read_whole_file_with_slow_start(make_fstream());
        BOOST_TEST_MESSAGE("Reading file yet again, should've recovered by now");
        read_while_file_at_full_speed(make_fstream());
    });
}
