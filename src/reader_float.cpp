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

#if defined(SCN_HEADER_ONLY) && SCN_HEADER_ONLY
#define SCN_READER_FLOAT_CPP
#endif

#include <scn/detail/args.h>
#include <scn/detail/reader.h>

#include <cerrno>
#include <clocale>

#if SCN_HAS_FLOAT_CHARCONV
#include <charconv>
#endif

SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wold-style-cast")

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wold-style-cast")
SCN_CLANG_IGNORE("-Wextra-semi-stmt")
SCN_CLANG_IGNORE("-Wreserved-identifier")

#include "fast_float/include/fast_float/fast_float.h"

SCN_CLANG_POP
SCN_GCC_POP

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace read_float {
        static bool is_hexfloat(const char* str, std::size_t len) noexcept
        {
            if (len < 3) {
                return false;
            }
            return str[0] == '0' && (str[1] == 'x' || str[1] == 'X');
        }

        namespace cstd {
#if SCN_GCC >= SCN_COMPILER(7, 0, 0)
            SCN_GCC_PUSH
            SCN_GCC_IGNORE("-Wnoexcept-type")
#endif
            template <typename T, typename CharT, typename F>
            expected<T> impl(F&& f_strtod,
                             T huge_value,
                             const CharT* str,
                             size_t& chars)
            {
                // Get current C locale
                const auto loc = std::setlocale(LC_NUMERIC, nullptr);
                // For whatever reason, this cannot be stored in the heap if
                // setlocale hasn't been called before, or msan errors with
                // 'use-of-unitialized-value' when resetting the locale back.
                // POSIX specifies that the content of loc may not be static, so
                // we need to save it ourselves
                char locbuf[256] = {0};
                std::strcpy(locbuf, loc);

                std::setlocale(LC_NUMERIC, "C");

                CharT* end{};
                errno = 0;
                T f = f_strtod(str, &end);
                chars = static_cast<size_t>(end - str);
                auto err = errno;
                // Reset locale
                std::setlocale(LC_NUMERIC, locbuf);
                errno = 0;

                SCN_GCC_COMPAT_PUSH
                SCN_GCC_COMPAT_IGNORE("-Wfloat-equal")
                // No conversion
                if (f == detail::zero_value<T>::value && chars == 0) {
                    return error(error::invalid_scanned_value, "strtod");
                }
                // Range error
                if (err == ERANGE) {
                    // Underflow
                    if (f == detail::zero_value<T>::value) {
                        return error(
                            error::value_out_of_range,
                            "Floating-point value out of range: underflow");
                    }
                    // Overflow
                    if (f == huge_value || f == -huge_value) {
                        return error(
                            error::value_out_of_range,
                            "Floating-point value out of range: overflow");
                    }
                    // Subnormals cause ERANGE but a value is still returned
                }
                SCN_GCC_COMPAT_POP
                return f;
            }
#if SCN_GCC >= SCN_COMPILER(7, 0, 0)
            SCN_GCC_POP
#endif

            template <typename CharT, typename T>
            struct read;

            template <>
            struct read<char, float> {
                static expected<float> get(const char* str, size_t& chars)
                {
                    return impl<float>(strtof, HUGE_VALF, str, chars);
                }
            };

            template <>
            struct read<char, double> {
                static expected<double> get(const char* str, size_t& chars)
                {
                    return impl<double>(strtod, HUGE_VAL, str, chars);
                }
            };

            template <>
            struct read<char, long double> {
                static expected<long double> get(const char* str, size_t& chars)
                {
                    return impl<long double>(strtold, HUGE_VALL, str, chars);
                }
            };

            template <>
            struct read<wchar_t, float> {
                static expected<float> get(const wchar_t* str, size_t& chars)
                {
                    return impl<float>(wcstof, HUGE_VALF, str, chars);
                }
            };
            template <>
            struct read<wchar_t, double> {
                static expected<double> get(const wchar_t* str, size_t& chars)
                {
                    return impl<double>(wcstod, HUGE_VAL, str, chars);
                }
            };
            template <>
            struct read<wchar_t, long double> {
                static expected<long double> get(const wchar_t* str,
                                                 size_t& chars)
                {
                    return impl<long double>(wcstold, HUGE_VALL, str, chars);
                }
            };
        }  // namespace cstd

        namespace from_chars {
#if SCN_HAS_FLOAT_CHARCONV
            template <typename T>
            struct read {
                static expected<T> get(const char* str, size_t& chars)
                {
                    const auto len = std::strlen(str);
                    auto flags = std::chars_format::general;
                    if (is_hexfloat(str, len)) {
                        str += 2;
                        flags = std::chars_format::hex;
                    }

                    T value{};
                    const auto result =
                        std::from_chars(str, str + len, value, flags);
                    if (result.ec == std::errc::invalid_argument) {
                        return error(error::invalid_scanned_value,
                                     "from_chars");
                    }
                    if (result.ec == std::errc::result_out_of_range) {
                        // Out of range, may be subnormal -> fall back to strtod
                        // On gcc std::from_chars doesn't parse subnormals
                        return cstd::read<char, T>::get(str, chars);
                    }
                    chars = static_cast<size_t>(result.ptr - str);
                    return value;
                }
            };
#else
            template <typename T>
            struct read {
                static expected<T> get(const char* str, size_t& chars)
                {
                    // Fall straight back to strtod
                    return cstd::read<char, T>::get(str, chars);
                }
            };
#endif
        }  // namespace from_chars

        namespace fast_float {
            template <typename T>
            expected<T> impl(const char* str, size_t& chars)
            {
                const auto len = std::strlen(str);
                if (is_hexfloat(str, len)) {
                    // fast_float doesn't support hexfloats
                    return from_chars::read<T>::get(str, chars);
                }

                T value{};
                const auto result =
                    ::fast_float::from_chars(str, str + len, value);
                if (result.ec == std::errc::invalid_argument) {
                    return error(error::invalid_scanned_value, "fast_float");
                }
                if (result.ec == std::errc::result_out_of_range) {
                    return error(error::value_out_of_range, "fast_float");
                }
                if (std::isinf(value)) {
                    // fast_float represents very large or small values as inf
                    // But, it also parses "inf", which from_chars does not
                    if (!(len >= 3 && (str[0] == 'i' || str[0] == 'I'))) {
                        // Input was not actually infinity ->
                        // invalid result, fall back to from_chars
                        return from_chars::read<T>::get(str, chars);
                    }
                }
                chars = static_cast<size_t>(result.ptr - str);
                return value;
            }

            template <typename T>
            struct read;

            template <>
            struct read<float> {
                static expected<float> get(const char* str, size_t& chars)
                {
                    return impl<float>(str, chars);
                }
            };
            template <>
            struct read<double> {
                static expected<double> get(const char* str, size_t& chars)
                {
                    return impl<double>(str, chars);
                }
            };
            template <>
            struct read<long double> {
                static expected<long double> get(const char* str, size_t& chars)
                {
                    // Fallback to strtod
                    // fast_float doesn't support long double
                    return cstd::read<char, long double>::get(str, chars);
                }
            };
        }  // namespace fast_float

        template <typename CharT, typename T>
        struct read;

        template <typename T>
        struct read<char, T> {
            static expected<T> get(const char* str, size_t& chars)
            {
                // char -> default to fast_float,
                // fallback to strtod if necessary
                return read_float::fast_float::read<T>::get(str, chars);
            }
        };
        template <typename T>
        struct read<wchar_t, T> {
            static expected<T> get(const wchar_t* str, size_t& chars)
            {
                // wchar_t -> straight to strtod
                return read_float::cstd::read<wchar_t, T>::get(str, chars);
            }
        };
    }  // namespace read_float

    namespace detail {
        template <typename T>
        template <typename CharT>
        expected<T> float_scanner<T>::_read_float_impl(const CharT* str,
                                                       size_t& chars)
        {
            // Parsing algorithm to use:
            // If CharT == wchar_t -> strtod
            // If CharT == char:
            //   1. fast_float
            //      fallback if a hex float, or incorrectly parsed an inf
            //      (very large or small value)
            //   2. std::from_chars
            //      fallback if not available (C++17) or float is subnormal
            //   3. std::strtod
            return read_float::read<CharT, T>::get(str, chars);
        }

        template expected<float> float_scanner<float>::_read_float_impl(
            const char*,
            size_t&);
        template expected<double> float_scanner<double>::_read_float_impl(
            const char*,
            size_t&);
        template expected<long double>
        float_scanner<long double>::_read_float_impl(const char*, size_t&);
        template expected<float> float_scanner<float>::_read_float_impl(
            const wchar_t*,
            size_t&);
        template expected<double> float_scanner<double>::_read_float_impl(
            const wchar_t*,
            size_t&);
        template expected<long double>
        float_scanner<long double>::_read_float_impl(const wchar_t*, size_t&);
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn