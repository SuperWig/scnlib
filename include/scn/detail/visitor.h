// Copyright 2017-2019 Elias Kosunen
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

#ifndef SCN_DETAIL_VISITOR_H
#define SCN_DETAIL_VISITOR_H

#include "reader.h"

namespace scn {
    SCN_BEGIN_NAMESPACE

    template <typename Context, typename ParseCtx>
    class basic_visitor {
    public:
        using context_type = Context;
        using char_type = typename Context::char_type;

        basic_visitor(Context& ctx, ParseCtx& pctx)
            : m_ctx(std::addressof(ctx)), m_pctx(std::addressof(pctx))
        {
        }

        template <typename T>
        auto operator()(T&& val) -> error
        {
            return visit(std::forward<T>(val), detail::priority_tag<1>{});
        }

    private:
        auto visit(char_type& val, detail::priority_tag<1>) -> error
        {
            detail::char_scanner s;
            auto err = parse(s);
            if (!err) {
                return err;
            }
            return s.scan(val, *m_ctx);
        }
        auto visit(span<char_type>& val, detail::priority_tag<1>) -> error
        {
            detail::buffer_scanner s;
            auto err = parse(s);
            if (!err) {
                return err;
            }
            return s.scan(val, *m_ctx);
        }
        auto visit(bool& val, detail::priority_tag<1>) -> error
        {
            detail::bool_scanner s;
            auto err = parse(s);
            if (!err) {
                return err;
            }
            return s.scan(val, *m_ctx);
        }

#define SCN_VISIT_INT(T)                         \
    error visit(T& val, detail::priority_tag<0>) \
    {                                            \
        detail::integer_scanner<T> s;            \
        auto err = parse(s);                     \
        if (!err) {                              \
            return err;                          \
        }                                        \
        return s.scan(val, *m_ctx);              \
    }
        SCN_VISIT_INT(short)
        SCN_VISIT_INT(int)
        SCN_VISIT_INT(long)
        SCN_VISIT_INT(long long)
        SCN_VISIT_INT(unsigned short)
        SCN_VISIT_INT(unsigned int)
        SCN_VISIT_INT(unsigned long)
        SCN_VISIT_INT(unsigned long long)
#undef SCN_VISIT_INT

#define SCN_VISIT_FLOAT(T)                       \
    error visit(T& val, detail::priority_tag<1>) \
    {                                            \
        detail::float_scanner<T> s;              \
        auto err = parse(s);                     \
        if (!err) {                              \
            return err;                          \
        }                                        \
        return s.scan(val, *m_ctx);              \
    }
        SCN_VISIT_FLOAT(float)
        SCN_VISIT_FLOAT(double)
        SCN_VISIT_FLOAT(long double)
#undef SCN_VISIT_FLOAT

        auto visit(std::basic_string<char_type>& val, detail::priority_tag<1>)
            -> error
        {
            detail::string_scanner s;
            auto err = parse(s);
            if (!err) {
                return err;
            }
            return s.scan(val, *m_ctx);
        }
        auto visit(basic_string_view<char_type>& val, detail::priority_tag<1>)
            -> error
        {
            detail::string_view_scanner s;
            auto err = parse(s);
            if (!err) {
                return err;
            }
            return s.scan(val, *m_ctx);
        }
        auto visit(typename Context::arg_type::handle val,
                   detail::priority_tag<1>) -> error
        {
            return val.scan(*m_ctx);
        }
        auto visit(detail::monostate, detail::priority_tag<0>) -> error
        {
            return error(error::invalid_operation, "Cannot scan a monostate");
        }

        template <typename Scanner>
        error parse(Scanner& s)
        {
            return m_pctx->parse(s, *m_pctx);
        }

        Context* m_ctx;
        ParseCtx* m_pctx;
    };

    template <typename ReturnType>
    class scan_result : public result<std::ptrdiff_t> {
    public:
        using return_type = ReturnType;
        using range_type = typename return_type::view_type;
        using base_type = result<std::ptrdiff_t>;

        SCN_CONSTEXPR scan_result(base_type&& b, return_type&& r)
            : base_type(std::move(b)), m_range(std::move(r))
        {
        }

        range_type& range() &
        {
            return m_range.get();
        }
        const range_type& range() const&
        {
            return m_range.get();
        }
        range_type&& range() &&
        {
            return std::move(m_range.get());
        }

        auto begin() -> decltype(detail::ranges::begin(range()))
        {
            return detail::ranges::begin(m_range);
        }
        auto begin() const -> decltype(detail::ranges::begin(range()))
        {
            return detail::ranges::begin(m_range);
        }
        auto cbegin() const -> decltype(detail::ranges::cbegin(range()))
        {
            return detail::ranges::cbegin(m_range);
        }

        auto end() -> decltype(detail::ranges::end(range()))
        {
            return detail::ranges::end(m_range);
        }
        auto end() const -> decltype(detail::ranges::end(range()))
        {
            return detail::ranges::end(m_range);
        }
        auto cend() const -> decltype(detail::ranges::cend(range()))
        {
            return detail::ranges::cend(m_range);
        }

    private:
        return_type m_range;
    };
    template <typename Context>
    struct scan_result_for {
        using type = scan_result<typename Context::range_type::return_type>;
    };
    template <typename Context>
    using scan_result_for_t = typename scan_result_for<Context>::type;

    template <typename Context, typename ParseCtx>
    scan_result_for_t<Context> visit(Context& ctx, ParseCtx& pctx)
    {
        std::ptrdiff_t args_read = 0;

        auto reterror = [&args_read,
                         &ctx](error e) -> scan_result_for_t<Context> {
            return {{args_read, std::move(e)}, ctx.range().get_return()};
        };

        auto arg = typename Context::arg_type();

        {
            auto ret = skip_range_whitespace(ctx);
            if (!ret) {
                return reterror(ret);
            }
        }

        while (pctx) {
            if (pctx.should_skip_ws()) {
                // Skip whitespace from format string and from stream
                // EOF is not an error
                auto ret = skip_range_whitespace(ctx);
                if (SCN_UNLIKELY(!ret)) {
                    if (ret == error::end_of_stream) {
                        break;
                    }
                    SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
                    auto rb = ctx.range().reset_to_rollback_point();
                    if (!rb) {
                        return reterror(rb);
                    }
                    return reterror(ret);
                }
                // Don't advance pctx, should_skip_ws() does it for us
                continue;
            }

            // Non-brace character, or
            // Brace followed by another brace, meaning a literal '{'
            if (pctx.should_read_literal()) {
                if (SCN_UNLIKELY(!pctx)) {
                    return reterror(error(error::invalid_format_string,
                                          "Unexpected end of format string"));
                }
                // Check for any non-specifier {foo} characters
                auto ret = read_char(ctx.range());
                SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
                if (!ret || !pctx.check_literal(ret.value())) {
                    auto rb = ctx.range().reset_to_rollback_point();
                    if (!rb) {
                        // Failed rollback
                        return reterror(rb);
                    }
                    if (!ret) {
                        // Failed read
                        return reterror(ret.error());
                    }

                    // Mismatching characters in scan string and stream
                    return reterror(
                        error(error::invalid_scanned_value,
                              "Expected character from format string not "
                              "found in the stream"));
                }
                // Bump pctx to next char
                pctx.advance();
            }
            else {
                // Scan argument
                auto id_wrapped = pctx.parse_arg_id();
                if (!id_wrapped) {
                    return reterror(id_wrapped.error());
                }
                auto id = id_wrapped.value();
                auto arg_wrapped = [&]() -> expected<typename Context::arg_type>
                {
                    if (id.empty()) {
                        return ctx.next_arg(pctx);
                    }
                    if (ctx.locale().is_digit(id.front())) {
                        std::ptrdiff_t tmp = 0;
                        for (auto ch : id) {
                            tmp =
                                tmp * 10 +
                                static_cast<std::ptrdiff_t>(
                                    ch - detail::ascii_widen<
                                             typename Context::char_type>('0'));
                        }
                        return ctx.arg(pctx, tmp);
                    }
                    return ctx.arg(id);
                }
                ();
                if (!arg_wrapped) {
                    return reterror(arg_wrapped.error());
                }
                arg = arg_wrapped.value();
                if (!pctx) {
                    return reterror(error(error::invalid_format_string,
                                          "Unexpected end of format argument"));
                }
                if (!arg) {
                    // Mismatch between number of args and {}s
                    return reterror(
                        error(error::invalid_format_string,
                              "Mismatch between number of arguments and "
                              "'{}' in the format string"));
                }
                auto ret = visit_arg<Context>(
                    basic_visitor<Context, ParseCtx>(ctx, pctx), arg);
                if (!ret) {
                    auto rb = ctx.range().reset_to_rollback_point();
                    if (!rb) {
                        return reterror(rb);
                    }
                    return reterror(ret);
                }
                // Handle next arg and bump pctx
                ++args_read;
                pctx.arg_handled();
                if (pctx) {
                    pctx.advance();
                }
            }
        }
        if (pctx) {
            // Format string not exhausted
            return reterror(error(error::invalid_format_string,
                                  "Format string not exhausted"));
        }
        ctx.range().set_rollback_point();
        return {args_read, ctx.range().get_return()};
    }

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_DETAIL_VISITOR_H
