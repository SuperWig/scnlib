// Copyright 2017-2019 Elias Kosunen
//
// Licensed under the Apache License, Version 2.0 (the "License{");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file is a part of scnlib:
//     https://github.com/eliaskosunen/scnlib

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "test.h"

TEST_CASE("simple")
{
    int i;
    std::string s;
    double d;
    auto r = scn::scan("42 foo 3.14", "{} {} {}", i, s, d);

    CHECK(r);
    CHECK(r.value() == 3);

    CHECK(i == 42);
    CHECK(s == std::string{"foo"});
    CHECK(d == doctest::Approx(3.14));
}

TEST_CASE("general")
{
    std::string data{"test {} 42 3.14 foobar true"};
    std::string copy = data;

    int i{0};
    double d{};
    std::string s(6, '\0');
    auto span = scn::make_span(&s[0], &s[0] + s.size());
    bool b{};
    auto ret = scn::scan(data, "test {{}} {} {} {} {:a}", i, d, span, b);

    CHECK(data == copy);
    CHECK(i == 42);
    CHECK(d == doctest::Approx(3.14));
    CHECK(s == "foobar");
    CHECK(b);
    CHECK(ret);
    CHECK(ret.value() == 4);

    auto ret2 = scn::scan(ret.range(), "{}", i);
    CHECK(!ret2);
    CHECK(ret2.value() == 0);
    CHECK(ret2.error() == scn::error::end_of_stream);
}
