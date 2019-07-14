//---------------------------------------------------------------------------//
// Copyright (c) 2018 John Maddock
// Copyright (c) 2018-2019 Nil Foundation AG
// Copyright (c) 2018-2019 Mikhail Komarov <nemo@nilfoundation.org>
// Copyright (c) 2018-2019 Alexey Moskvin
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#ifndef BOOST_MULTIPRECISION_MODULAR_ADAPTOR_HPP
#define BOOST_MULTIPRECISION_MODULAR_ADAPTOR_HPP

#include <boost/cstdint.hpp>
#include <boost/functional/hash_fwd.hpp>
#include <boost/multiprecision/detail/digits.hpp>
#include <boost/multiprecision/montgomery_params.hpp>
#include <boost/multiprecision/number.hpp>
#include <boost/type_traits/is_complex.hpp>

#include <algorithm>
#include <cmath>
#include <complex>

namespace boost {
namespace multiprecision {
namespace backends {

template <class BackendBase, class BackendMod> struct modular_adaptor {
protected:
  BackendBase m_base;
  montgomery_params<BackendMod> m_mod;

public:
  BackendBase &base_data() { return m_base; }

  const BackendBase &base_data() const { return m_base; }

  montgomery_params<BackendMod> &mod_data() { return m_mod; }

  const montgomery_params<BackendMod> &mod_data() const { return m_mod; }

  // typedef typename Backend::signed_types     signed_types;
  // typedef typename Backend::unsigned_types   unsigned_types;
  // typedef typename Backend::float_types      float_types;
  // typedef typename Backend::exponent_type    exponent_type;

  modular_adaptor() {}

  modular_adaptor(const modular_adaptor &o)
      : m_base(o.base_data()), m_mod(o.mod_data()) {}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

  modular_adaptor(modular_adaptor &&o)
      : m_base(std::move(o.base_data())), m_mod(std::move(o.mod_data())) {}

#endif

  modular_adaptor(const BackendBase &val) : m_base(val) {}

  modular_adaptor(const BackendBase &val,
                  const montgomery_params<BackendMod> &mod)
      : m_base(val), m_mod(mod) {}

  modular_adaptor(const montgomery_params<BackendMod> &mod)
      : m_base(0), m_mod(mod) {}

  modular_adaptor &operator=(const modular_adaptor &o) {
    m_base = o.base_data();
    m_mod = o.mod_data();
    return *this;
  }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

  modular_adaptor &operator=(modular_adaptor &&o)

      BOOST_NOEXCEPT {
    m_base = std::move(o.base_data());
    m_mod = std::move(o.mod_data());
    return *this;
  }
#endif

  int compare(const modular_adaptor &o) const {
    // They are either equal or not:
    return (m_base.compare(o.base_data()) == 0) &&
                   (m_mod.compare(o.mod_data()) == 0)
               ? 0
               : 1;
  }

  template <class T> int compare(const T &val) const {
    using default_ops::eval_lt;
    return (m_base.compare(val) == 0) && eval_lt(m_mod, val) ? 0 : 1;
  }

  void swap(modular_adaptor &o) {
    base_data().swap(o.base_data());
    mod_data().swap(o.mod_data());
  }

  std::string str(std::streamsize dig, std::ios_base::fmtflags f) const {
    modular_adaptor<BackendBase, montgomery_params<BackendMod>> tmp = this;
    redc(tmp);
    return str(tmp.base_data());
  }
};

template <class BackendBase, class BackendMod, class T>
inline typename enable_if<is_arithmetic<T>, bool>

    ::type eval_eq(const modular_adaptor<BackendBase, BackendMod> &a,
                   const T &b)

        BOOST_NOEXCEPT {
  return a.compare(b) == 0;
}

template <class BackendBase, class BackendMod>
inline void eval_add(modular_adaptor<BackendBase, BackendMod> &result,
                     const modular_adaptor<BackendBase, BackendMod> &o) {
  eval_add(result.base_data(), o.base_data());
  redc(result);
}

template <class BackendBase, class BackendMod>
inline void eval_subtract(modular_adaptor<BackendBase, BackendMod> &result,
                          const modular_adaptor<BackendBase, BackendMod> &o) {
  BackendBase tmp = result.base_data();
  eval_subtract(tmp, o.base_data());
  if (eval_lt(tmp, 0)) {
    BOOST_THROW_EXCEPTION(std::range_error("Montgomery less than zero"));
  }
  result.base_data() = tmp;
}

template <class BackendBase, class BackendMod>
inline void eval_multiply(modular_adaptor<BackendBase, BackendMod> &result,
                          const modular_adaptor<BackendBase, BackendMod> &o) {
  eval_multiply(result.base_data(), o.base_data());
  redc(result);
}

template <class BackendBase, class BackendMod>
inline void eval_divide(modular_adaptor<BackendBase, BackendMod> &result,
                        const modular_adaptor<BackendBase, BackendMod> &o) {
  eval_divide(result.base_data(), o.base_data());
}

template <class BackendBase, class BackendMod>
inline bool eval_is_zero(const modular_adaptor<BackendBase, BackendMod> &val)

    BOOST_NOEXCEPT {
  using default_ops::eval_is_zero;
  return eval_is_zero(val.

                      base_data()

  );
}

template <class BackendBase, class BackendMod>
inline int eval_get_sign(const modular_adaptor<BackendBase, BackendMod> &) {
  return 1;
}

template <class Result, class BackendBase, class BackendMod>
inline typename disable_if_c<boost::is_complex<Result>::value>::type
eval_convert_to(Result *result,
                const modular_adaptor<BackendBase, BackendMod> &val) {
  using default_ops::eval_convert_to;
  using default_ops::eval_is_zero;
  if (!eval_is_zero(val.imag_data())) {
    BOOST_THROW_EXCEPTION(
        std::runtime_error("Could not convert imaginary number to scalar."));
  }
  eval_convert_to(result, val.real_data());
}

template <class BackendBase, class BackendMod, class T, class V>
inline void assign_components(modular_adaptor<BackendBase, BackendMod> &result,
                              const T &a, const montgomery_params<V> &b) {
  result.base_data() = a;
  result.mod_data() = b;
}

template <class BackendBase, class BackendMod>
inline void eval_sqrt(modular_adaptor<BackendBase, BackendMod> &result,
                      const modular_adaptor<BackendBase, BackendMod> &val) {
  eval_sqrt(result.base_data(), val.base_data());
}

template <class BackendBase, class BackendMod>
inline void eval_abs(modular_adaptor<BackendBase, BackendMod> &result,
                     const modular_adaptor<BackendBase, BackendMod> &val) {
  result = val;
}

template <class BackendBase, class BackendMod>
inline void eval_pow(modular_adaptor<BackendBase, BackendMod> &result,
                     const modular_adaptor<BackendBase, BackendMod> &b,
                     const modular_adaptor<BackendBase, BackendMod> &e) {
  using default_ops::eval_acos;
  using default_ops::eval_exp;
  modular_adaptor<BackendBase, BackendMod> t;
  eval_log(t, b);
  eval_multiply(t, e);
  eval_exp(result, t);
  redc(result);
}

template <class BackendBase, class BackendMod>
inline void eval_exp(modular_adaptor<BackendBase, BackendMod> &result,
                     const modular_adaptor<BackendBase, BackendMod> &arg) {
  using default_ops::eval_exp;

  eval_exp(result.base_data(), arg.base_data());
  result.mod_data() = arg.mod_data();
  redc(result);
}

template <class BackendBase, class BackendMod>
inline void eval_log(modular_adaptor<BackendBase, BackendMod> &result,
                     const modular_adaptor<BackendBase, BackendMod> &arg) {
  using default_ops::eval_log;

  eval_log(result.base_data(), arg.base_data());
  result.mod_data() = arg.mod_data();
  redc(result);
}

} // namespace backends

} // namespace multiprecision

#endif
