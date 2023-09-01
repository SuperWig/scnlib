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

#pragma once

#include <scn/detail/scanner.h>
#include <scn/impl/algorithms/read.h>
#include <scn/impl/reader/common.h>
#include <scn/impl/reader/integer_reader.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        template <typename CharT>
        class code_unit_reader {
        public:
            template <typename SourceRange>
            scan_expected<simple_borrowed_iterator_t<SourceRange>> read(
                SourceRange&& range,
                CharT& ch)
            {
                return read_code_unit(range).transform(
                    [&](auto it) SCN_NOEXCEPT {
                        ch = *ranges::begin(range);
                        return it;
                    });
            }
        };

        template <typename CharT>
        class code_point_reader;

        template <>
        class code_point_reader<code_point> {
        public:
            template <typename SourceRange>
            scan_expected<simple_borrowed_iterator_t<SourceRange>> read(
                SourceRange&& range,
                code_point& cp)
            {
                return read_code_point_into(SCN_FWD(range))
                    .transform([&](auto result) {
                        cp = decode_code_point_exhaustive_valid(
                            result.value.view());
                        return result.iterator;
                    });
            }
        };

        template <>
        class code_point_reader<wchar_t> {
        public:
            template <typename SourceRange>
            scan_expected<simple_borrowed_iterator_t<SourceRange>> read(
                SourceRange&& range,
                wchar_t& ch)
            {
                code_point_reader<code_point> reader{};
                code_point cp{};
                auto ret = reader.read(SCN_FWD(range), cp);
                if (!ret) {
                    return unexpected(ret.error());
                }

                return encode_code_point_as_wide_character(cp, true).transform(
                    [&](auto encoded_ch) SCN_NOEXCEPT {
                        ch = encoded_ch;
                        return *ret;
                    });
            }
        };

        template <typename SourceCharT, typename ValueCharT>
        class char_reader_base {
        public:
            constexpr char_reader_base() = default;

            bool skip_ws_before_read() const
            {
                return false;
            }

            static scan_error check_specs(
                const detail::basic_format_specs<SourceCharT>& specs)
            {
                reader_error_handler eh{};
                if constexpr (std::is_same_v<ValueCharT, code_point>) {
                    detail::check_code_point_type_specs(specs, eh);
                }
                else {
                    detail::check_char_type_specs(specs, eh);
                }
                if (SCN_UNLIKELY(!eh)) {
                    return {scan_error::invalid_format_string, eh.m_msg};
                }
                return {};
            }
        };

        template <typename CharT>
        class reader_impl_for_char : public char_reader_base<CharT, char> {
        public:
            template <typename Range>
            scan_expected<ranges::iterator_t<Range>>
            read_default(Range range, char& value, detail::locale_ref loc)
            {
                SCN_UNUSED(loc);
                if constexpr (std::is_same_v<CharT, char>) {
                    return code_unit_reader<char>{}.read(range, value);
                }
                else {
                    SCN_UNUSED(range);
                    SCN_EXPECT(false);
                    SCN_UNREACHABLE;
                }
            }

            template <typename Range>
            scan_expected<ranges::iterator_t<Range>> read_specs(
                Range range,
                const detail::basic_format_specs<CharT>& specs,
                char& value,
                detail::locale_ref loc)
            {
                if (specs.type == detail::presentation_type::none ||
                    specs.type == detail::presentation_type::character) {
                    return read_default(range, value, loc);
                }

                reader_impl_for_int<CharT> reader{};
                signed char tmp_value{};
                auto ret = reader.read_specs(range, specs, tmp_value, loc);
                value = static_cast<signed char>(value);
                return ret;
            }
        };

        template <typename CharT>
        class reader_impl_for_wchar : public char_reader_base<CharT, wchar_t> {
        public:
            template <typename Range>
            scan_expected<ranges::iterator_t<Range>>
            read_default(Range range, wchar_t& value, detail::locale_ref loc)
            {
                SCN_UNUSED(loc);
                if constexpr (std::is_same_v<CharT, char>) {
                    return code_point_reader<wchar_t>{}.read(range, value);
                }
                else {
                    return code_unit_reader<wchar_t>{}.read(range, value);
                }
            }

            template <typename Range>
            scan_expected<ranges::iterator_t<Range>> read_specs(
                Range range,
                const detail::basic_format_specs<CharT>& specs,
                wchar_t& value,
                detail::locale_ref loc)
            {
                if (specs.type == detail::presentation_type::none ||
                    specs.type == detail::presentation_type::character) {
                    return read_default(range, value, loc);
                }

                reader_impl_for_int<CharT> reader{};
                using integer_type =
                    std::conditional_t<sizeof(wchar_t) == 2, int16_t, int32_t>;
                integer_type tmp_value{};
                auto ret = reader.read_specs(range, specs, tmp_value, loc);
                value = static_cast<integer_type>(value);
                return ret;
            }
        };

        template <typename CharT>
        class reader_impl_for_code_point
            : public char_reader_base<CharT, code_point> {
        public:
            template <typename Range>
            scan_expected<ranges::iterator_t<Range>>
            read_default(Range range, code_point& value, detail::locale_ref loc)
            {
                SCN_UNUSED(loc);
                return code_point_reader<code_point>{}.read(range, value);
            }

            template <typename Range>
            scan_expected<ranges::iterator_t<Range>> read_specs(
                Range range,
                const detail::basic_format_specs<CharT>& specs,
                code_point& value,
                detail::locale_ref loc)
            {
                SCN_UNUSED(specs);
                return read_default(range, value, loc);
            }
        };
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
