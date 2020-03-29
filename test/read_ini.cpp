//---------------------------------------------------------------------------//
// Copyright (c) 2011-2018 Dominik Charousset
// Copyright (c) 2017-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#define BOOST_TEST_MODULE detail.parser.read_ini

#include <nil/actor/detail/parser/read_ini.hpp>

#include <nil/actor/test/dsl.hpp>

#include <string>
#include <vector>

#include <nil/actor/config_value.hpp>
#include <nil/actor/parser_state.hpp>
#include <nil/actor/pec.hpp>
#include <nil/actor/string_view.hpp>

using namespace nil::actor;

namespace {

    using log_type = std::vector<std::string>;

    struct test_consumer {
        log_type log;

        test_consumer() = default;
        test_consumer(const test_consumer &) = delete;
        test_consumer &operator=(const test_consumer &) = delete;

        test_consumer &begin_map() {
            log.emplace_back("{");
            return *this;
        }

        void end_map() {
            log.emplace_back("}");
        }

        test_consumer &begin_list() {
            log.emplace_back("[");
            return *this;
        }

        void end_list() {
            log.emplace_back("]");
        }

        void key(std::string name) {
            add_entry("key: ", std::move(name));
        }

        template<class T>
        void value(T x) {
            config_value cv {std::move(x)};
            std::string entry = "value (";
            entry += cv.type_name();
            entry += "): ";
            entry += to_string(cv);
            log.emplace_back(std::move(entry));
        }

        void add_entry(const char *prefix, std::string str) {
            str.insert(0, prefix);
            log.emplace_back(std::move(str));
        }
    };

    struct ini_consumer {
        using inner_map = std::map<std::string, config_value>;
        using section_map = std::map<std::string, inner_map>;

        section_map sections;
        section_map::iterator current_section;
    };

    struct fixture {
        expected<log_type> parse(string_view str, bool expect_success = true) {
            test_consumer f;
            string_parser_state res {str.begin(), str.end()};
            detail::parser::read_ini(res, f);
            if ((res.code == pec::success) != expect_success) {
                BOOST_TEST_MESSAGE("unexpected parser result state: " << res.code);
                BOOST_TEST_MESSAGE("input remainder: " << std::string(res.i, res.e));
            }
            return std::move(f.log);
        }
    };

    template<class... Ts>
    log_type make_log(Ts &&... xs) {
        return log_type {std::forward<Ts>(xs)...};
    }

    // Tests basic functionality.
    constexpr const string_view ini0 = R"(
[1group]
1value=321
[_foo]
_bar=11
[logger]
padding= 10
file-name = "foobar.ini" ; our file name

[scheduler] ; more settings
  timing  =  2us ; using microsecond resolution
x_ =.123
some-bool=true
some-other-bool=false
some-list=[
; here we have some list entries
123,
  1..3,
  23 ; twenty-three!
  ,2..4..2,
  "abc", ; some comment and a trailing comma
]
some-map{
; here we have some list entries
entry1=123,
  entry2=23 ; twenty-three! btw, comma is not mandatory
 entry3= "abc" , ; some comment and a trailing comma
}
[middleman]
preconnect=[<
tcp://localhost:8080

   >,<udp://remotehost?trust=false>]
)";

    // clang-format off
const auto ini0_log = make_log(
  "key: 1group",
  "{",
    "key: 1value",
    "value (integer): 321",
  "}",
  "key: _foo",
  "{",
    "key: _bar",
    "value (integer): 11",
  "}",
  "key: logger",
  "{",
    "key: padding",
    "value (integer): 10",
    "key: file-name",
    "value (string): \"foobar.ini\"",
  "}",
  "key: scheduler",
  "{",
    "key: timing",
    "value (timespan): 2us",
    "key: x_",
    "value (real): " + deep_to_string(.123),
    "key: some-bool",
    "value (boolean): true",
    "key: some-other-bool",
    "value (boolean): false",
    "key: some-list",
    "[",
      "value (integer): 123",
      "value (integer): 1",
      "value (integer): 2",
      "value (integer): 3",
      "value (integer): 23",
      "value (integer): 2",
      "value (integer): 4",
      "value (string): \"abc\"",
    "]",
    "key: some-map",
    "{",
      "key: entry1",
      "value (integer): 123",
      "key: entry2",
      "value (integer): 23",
      "key: entry3",
      "value (string): \"abc\"",
    "}",
  "}",
  "key: middleman",
  "{",
    "key: preconnect",
    "[",
      "value (uri): tcp://localhost:8080",
      "value (uri): udp://remotehost?trust=false",
    "]",
  "}"
);
    // clang-format on

    // Tests nested parameters.
    constexpr const string_view ini1 = R"(
foo {
  bar = {
    value1 = 1
  }
  value2 = 2
}
[bar.foo]
value3 = 3
)";

    // clang-format off
const auto ini1_log = make_log(
  "key: global",
  "{",
    "key: foo",
    "{",
      "key: bar",
      "{",
        "key: value1",
        "value (integer): 1",
      "}",
      "key: value2",
      "value (integer): 2",
    "}",
  "}",
  "key: bar",
  "{",
    "key: foo",
    "{",
      "key: value3",
      "value (integer): 3",
    "}",
  "}"
);
    // clang-format on

    constexpr const string_view ini2 = "#";

    const auto ini2_log = make_log();

    constexpr const string_view ini3 = "; foobar\n!";

    const auto ini3_log = make_log();

}    // namespace

BOOST_FIXTURE_TEST_SUITE(read_ini_tests, fixture)

BOOST_AUTO_TEST_CASE(empty_inis) {
    BOOST_CHECK_EQUAL(parse(";foo"), make_log());
    BOOST_CHECK_EQUAL(parse(""), make_log());
    BOOST_CHECK_EQUAL(parse("  "), make_log());
    BOOST_CHECK_EQUAL(parse(" \n "), make_log());
    BOOST_CHECK_EQUAL(parse(";hello\n;world"), make_log());
}

BOOST_AUTO_TEST_CASE(section with valid key - value pairs) {
    BOOST_CHECK_EQUAL(parse("[foo]"), make_log("key: foo", "{", "}"));
    BOOST_CHECK_EQUAL(parse("  [foo]"), make_log("key: foo", "{", "}"));
    BOOST_CHECK_EQUAL(parse("  [  foo]  "), make_log("key: foo", "{", "}"));
    BOOST_CHECK_EQUAL(parse("  [  foo  ]  "), make_log("key: foo", "{", "}"));
    BOOST_CHECK_EQUAL(parse("\n[a-b];foo\n;bar"), make_log("key: a-b", "{", "}"));
    BOOST_CHECK_EQUAL(parse(ini0), ini0_log);
    BOOST_CHECK_EQUAL(parse(ini1), ini1_log);
}

BOOST_AUTO_TEST_CASE(invalid_inis) {
    BOOST_CHECK_EQUAL(parse(ini2), ini2_log);
    BOOST_CHECK_EQUAL(parse(ini3), ini3_log);
}

BOOST_AUTO_TEST_CASE(integer keys are legal in INI syntax) {
    static constexpr string_view ini = R"__(
    [foo.bar]
    1 = 10
    2 = 20
  )__";
    // clang-format off
  auto log = make_log(
    "key: foo",
    "{",
      "key: bar",
      "{",
        "key: 1",
        "value (integer): 10",
        "key: 2",
        "value (integer): 20",
      "}",
    "}"
  );
    // clang-format on
    BOOST_CHECK_EQUAL(parse(ini), log);
}

BOOST_AUTO_TEST_CASE(integer_keys_are_legal_in_config_syntax) {
    static constexpr string_view ini = R"__(
    foo {
      bar {
        1 = 10
        2 = 20
      }
    }
  )__";
    // clang-format off
  auto log = make_log(
    "key: global",
    "{",
      "key: foo",
      "{",
        "key: bar",
        "{",
          "key: 1",
          "value (integer): 10",
          "key: 2",
          "value (integer): 20",
        "}",
      "}",
    "}"
  );
    // clang-format on
    BOOST_CHECK_EQUAL(parse(ini), log);
}

BOOST_AUTO_TEST_SUITE_END()
