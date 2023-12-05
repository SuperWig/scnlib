// Copyright 2017 Elias Kosunen
//
// Licensed under the Apache License, Version 2.0 (the "License");
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

#include "wrapped_gtest.h"

#include <scn/detail/scan_buffer.h>

using namespace std::string_view_literals;

TEST(ScanBufferTest, StringView)
{
    auto buf = scn::detail::make_string_scan_buffer("foobar"sv);
    static_assert(std::is_same_v<decltype(buf),
                                 scn::detail::basic_scan_string_buffer<char>>);

    EXPECT_TRUE(buf.is_contiguous());
    EXPECT_EQ(buf.characters_read(), 6);
    EXPECT_EQ(buf.get_contiguous_segment(), "foobar");
    EXPECT_EQ(buf.get_contiguous_buffer(), "foobar");
}

TEST(ScanBufferTest, TakeStringView)
{
    auto range = scn::ranges::take_view("foobar"sv, 6);
    auto buf = scn::detail::make_forward_scan_buffer(range);
    static_assert(
        std::is_same_v<
            decltype(buf),
            scn::detail::basic_scan_forward_buffer_impl<decltype(range)>>);

    EXPECT_FALSE(buf.is_contiguous());
    EXPECT_EQ(buf.characters_read(), 0);

    auto view = buf.get_forward_buffer();
    auto it = view.begin();
    EXPECT_NE(it, view.end());
    EXPECT_EQ(*it, 'f');
    ++it;
    EXPECT_NE(it, view.end());
    EXPECT_EQ(*it, 'o');
    ++it;
    EXPECT_NE(it, view.end());

    std::string dest;
    scn::ranges::copy(buf.get_forward_buffer(), std::back_inserter(dest));
    EXPECT_EQ(dest, "foobar");
    EXPECT_EQ(buf.characters_read(), 6);
    EXPECT_EQ(buf.get_contiguous_segment(), "foobar");
}

TEST(ScanBufferTest, ReverseStringView)
{
    auto range = scn::ranges::reverse_view("foobar"sv);
    auto buf = scn::detail::make_forward_scan_buffer(range);
    static_assert(
        std::is_same_v<
            decltype(buf),
            scn::detail::basic_scan_forward_buffer_impl<decltype(range)>>);

    EXPECT_FALSE(buf.is_contiguous());
    EXPECT_EQ(buf.characters_read(), 0);

    std::string dest;
    scn::ranges::copy(buf.get_forward_buffer(), std::back_inserter(dest));
    EXPECT_EQ(dest, "raboof");
    EXPECT_EQ(buf.characters_read(), 6);
    EXPECT_EQ(buf.get_contiguous_segment(), "raboof");
}