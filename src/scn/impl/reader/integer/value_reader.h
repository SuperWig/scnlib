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
#include <scn/impl/reader/common.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        struct int_value_reader_base {
            enum options_type {
                // ' option -> accept thsep (',')
                allow_thsep = 1,
                // 'u' option -> don't allow sign
                only_unsigned = 2,
                // Allow base prefix (e.g. 0B or 0x)
                allow_base_prefix = 4,
            };

            int_value_reader_base() = default;
            int_value_reader_base(unsigned opt, int b)
                : m_options(opt), m_base(b)
            {
            }

            template <typename T>
            static constexpr unsigned get_default_options()
            {
                return 0;
            }

            unsigned m_options{0};
            int m_base{0};
        };

        template <typename CharT>
        class int_classic_value_reader : public int_value_reader_base {
        public:
            using char_type = CharT;
            using string_view_type = std::basic_string_view<CharT>;

            int_classic_value_reader(unsigned opt, int b)
                : int_value_reader_base(opt, b)
            {
            }

            template <typename T>
            explicit int_classic_value_reader(detail::tag_type<T>)
                : int_value_reader_base(get_default_options<T>(), 0)
            {
            }

            template <typename T>
            scan_expected<ranges::iterator_t<string_view_type>> read(
                string_view_type source,
                T& value);
        };

        template <typename CharT>
        class int_localized_value_reader : public int_value_reader_base {
        public:
            using char_type = CharT;
            using string_view_type = std::basic_string_view<CharT>;

            int_localized_value_reader() = default;
            int_localized_value_reader(detail::locale_ref loc,
                                       unsigned opt = 0,
                                       int b = 0)
                : int_value_reader_base(opt, b), m_locale(loc)
            {
            }

            template <typename T>
            scan_expected<ranges::iterator_t<string_view_type>> read(
                string_view_type source,
                T& value);

        private:
            detail::locale_ref m_locale{};
        };

#define SCN_DECLARE_INT_VALUE_READER_TEMPLATE_IMPL(CharT, IntT)   \
    extern template auto int_classic_value_reader<CharT>::read(   \
        string_view_type, IntT&)                                  \
        ->scan_expected<ranges::iterator_t<string_view_type>>;    \
    extern template auto int_localized_value_reader<CharT>::read( \
        string_view_type, IntT&)                                  \
        ->scan_expected<ranges::iterator_t<string_view_type>>;

#define SCN_DECLARE_INT_VALUE_READER_TEMPLATE(CharT)                  \
    SCN_DECLARE_INT_VALUE_READER_TEMPLATE_IMPL(CharT, signed char)    \
    SCN_DECLARE_INT_VALUE_READER_TEMPLATE_IMPL(CharT, short)          \
    SCN_DECLARE_INT_VALUE_READER_TEMPLATE_IMPL(CharT, int)            \
    SCN_DECLARE_INT_VALUE_READER_TEMPLATE_IMPL(CharT, long)           \
    SCN_DECLARE_INT_VALUE_READER_TEMPLATE_IMPL(CharT, long long)      \
    SCN_DECLARE_INT_VALUE_READER_TEMPLATE_IMPL(CharT, unsigned char)  \
    SCN_DECLARE_INT_VALUE_READER_TEMPLATE_IMPL(CharT, unsigned short) \
    SCN_DECLARE_INT_VALUE_READER_TEMPLATE_IMPL(CharT, unsigned int)   \
    SCN_DECLARE_INT_VALUE_READER_TEMPLATE_IMPL(CharT, unsigned long)  \
    SCN_DECLARE_INT_VALUE_READER_TEMPLATE_IMPL(CharT, unsigned long long)

        SCN_DECLARE_INT_VALUE_READER_TEMPLATE(char)
        SCN_DECLARE_INT_VALUE_READER_TEMPLATE(wchar_t)

#undef SCN_DECLARE_INT_VALUE_READER_TEMPLATE
#undef SCN_DECLARE_INT_VALUE_READER_TEMPLATE_IMPL
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn