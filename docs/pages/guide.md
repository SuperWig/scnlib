\page guide Guide
\tableofcontents

\section basic Basic usage

`scn::scan` can be used to scan various values from a source range.

A range is an object that has a beginning and an end.
Examples of ranges are string literals, `std::string` and `std::vector<char>`.
Objects of these types, and more, can be passed to `scn::scan`.
To learn more about the requirements on these ranges, see the API documentation on source ranges.

After the source range, `scn::scan` is passed a format string.
This is similar in nature to `std::scanf`, and has virtually the same syntax as `std::format` and {fmt}.
In the format string, arguments are marked with curly braces `{}`.
Each `{}` means that a single value is to be scanned from the source range.
Because scnlib uses templates, type information is not required in the format string,
like it is with `std::scanf` (e.g. `%d`).

The list of the types of the values of the scan are given as template parameters to `scn::scan`.
`scn::scan` returns an object, which contains the read value.
If only a single value is read, it can be accessed through the member function `value()`,
otherwise all the read values can be accessed through a `std::tuple` with `values()`.

\code{.cpp}
// Scanning an int
auto result = scn::scan<int>("123", "{}"):
auto i = result->value();
// i == 123

// Scanning a double
auto result = scn::scan<double>("3.14", "{}");
auto& [d] = result->values();
// d == 3.14

// Scanning multiple values
auto result = scn::scan<int, int>("0 1 2", "{} {}");
auto& [a, b] = result->values();
// a == 0
// b == 1
// Note, that " 2" was not scanned,
// because only two integers were requested

// Scanning a string means scanning a "word" --
//   that is, until the next whitespace character
// this is the same behavior as with iostreams
auto result = scn::scan<std::string>("hello world", "{}");
// result->value() == "hello"
\endcode

Compare the above example to the same implemented with `std::istringstream`:

\code{.cpp}
int i;
std::istringstream{"123"} >> i;

double d;
std::istringstream{"3.14"} >> d;

int a, b;
std::istringstream{"0 1 2"} >> a >> b;

std::string str;
std::istringstream{"hello world"} >> str;
\endcode

Or with `std::sscanf`:

\code{.cpp}
int i;
std::sscanf("123", "%d", &i);

double d;
std::sscanf("3.14", "%lf", &d);

int a, b;
std::sscanf("0 1 2", "%d %d", &a, &b);

// Not really possible with scanf!
char buf[16] = {0};
std::sscanf("hello world", "%15s", buf);
// buf == "hello"
\endcode

\section errors Error handling and return values

scnlib does not use exceptions.
The library compiles with `-fno-exceptions -fno-rtti` and is perfectly usable without them.

Instead, it uses return values to signal errors: `scn::scan` returns an `scan_expected`.
This return value is truthy if the operation succeeded.
If there was an error, the `.error()` member function can be used to gather more details about the error.

The actual read values are accessed with either
`operator->` or member function `.value()` of the returned `scan_expected`.
This ensures, that if an error occurred, the values are not accidentally accessed.

\code{.cpp}
// "foo" is not an integer
auto result = scn::scan<int>("foo", "{}");
// fails, result->value() would be UB, result.value().value() would throw
if (!result) {
    std::cout << result.error().msg() << '\n';
}
\endcode

Unlike with `std::scanf`, partial successes are not supported.
Either the entire scanning operation succeeds, or a failure is returned.

\code{.cpp}
// "foo" is still not an integer
auto result = scn::scan<int, int>("123 foo", "{} {}");
// fails -- result == false
\endcode

Oftentimes, the entire source range is not scanned, and the remainder of the range may be useful later.
The unparsed input can be accessed with `->range()`, which returns a `subrange`.
An iterator pointing to the first unparsed element can be retrieved with `->begin()`.

\code{.cpp}
auto result = scn::scan<int>("123 456"sv, "{}");
// result == true
// result->value() == 123
// result->range() == " 456"

auto [other_result, i] = scn::scan<int>(result->range(), "{}");
// other_result == true
// i == 456
// other_result-> == ""
\endcode

The return type of `->range()` is a view into the range `scn::scan` was given.
Its type may not be the same as the source range, but its iterator and sentinel types are the same.
If the range given to `scn::scan` does not model `ranges::borrowed_range`
(essentially, the returned range would dangle), the returned range is of type `ranges::dangling`.

Because the range type returned by `scn::scan` is always a `subrange` over its input,
it's easy to use `scn::scan` in loops, as long as the input type is a `subrange` to begin with.
If it's not, consider making it one with `scn::ranges::subrange{your-input-range}`.

\code{.cpp}
auto input = scn::ranges::subrange{...};
while (auto result = scn::scan<...>(input, ...)) {
    // use result
    input = result->range();
}
\endcode

\section stdin Standard streams and `stdin`

To read from `stdin`, use `scn::input` or `scn::prompt`.
They work similarly to `scn::scan`, except they do not take an input range as a parameter: `stdin` is implied.
They take care of synchronization with `std::cin` and C stdio `stdin` buffers,
so `scn::input` usage can be mixed with both `std::cin` and `std::scanf`.
`scn::input` does not return a leftover range type.

\code{.cpp}
if (auto result = scn::input<int>("{}")) {
    // ...
}
// scn::input, std::cin, and std::scanf can be used immediately,
// without explicit synchronization
if (auto result = scn::prompt<int>("Provide a number: ", "{}"); result) {
    // ...
}
\endcode

`scn::input` is internally implemented by wrapping `std::cin` inside an `scn::istreambuf_view`.
`scn::istreambuf_view` is a view, that wraps a `std::streambuf`, and provides a range-like interface for it.
`scn::istreambuf_view` has a member function, `sync`,
that can be used to synchronize its state with the underlying `std::streambuf`,
so that it can be used again.

\code{.cpp}
std::istringstream ss{"123 456"};
auto ssview = scn::istreambuf_view{ss};
auto result = scn::scan<int>(ssview, "{}");
// result->value() == 123

result->begin().sync();
// ss can now be used again
int j{};
ss >> j;
\endcode

\section format Format string

Parsing of a given value can be customized with the format string.
The format string syntax is based on the one used by {fmt} and `std::format`.

In short, in the format string, `{}` represents a value to be parsed.
The type of the value is determined by the list of types given to `scn::scan`.

Any whitespace character in the format string is an instruction to skip all whitespace.
Some types may do that automatically.
This behavior is identical to `scanf`.

\code{.cpp}
// scanning a char doesn't automatically skip whitespace,
// int does
auto result = scn::scan<char, char, int>("x   123", "{}{}{}");
auto& [a, b, i] = result->values();
// a == 'x'
// b == ' '
// i == 123

// Whitespace in format string, skip all whitespace
auto result = scn::scan<char, char>("x        y", "{} {}");
auto& [a, b] = result->values();
// a == 'x'
// b == 'y'
\endcode

Any other character in the format string is expected to be found in the source range, and is then discarded.

\code{.cpp}
auto result = scn::scan<char>("abc", "ab{}");
// result->value() == 'c'
\endcode

Inside the curly braces `{}`, flags can be specified, that govern the way the value is parsed.
The flags start with a colon `:` character.
See the API Documentation for full reference on format string flags.

\code{.cpp}
// accept only hex floats
auto result = scn::scan<double>(..., "{:a}");

// interpret the parsed number as hex
auto result = scn::scan<int>(..., "{:x}");
\endcode

\section scan_value `scn::scan_value`

For simple cases, there's `scn::scan_value`.
It can be used to scan a single value from a source range, as if by using the default format string `"{}"`.

\code{.cpp}
auto result = scn::scan_value<int>("123");
// result->value() == 123
// result->range() is empty
\endcode

\section unicode Unicode and wide source ranges

scnlib expects all input given to it to be Unicode.
All input with the character/value type of `char` is always assumed to be UTF-8.
Encoding errors are checked for, and may cause scanning to fail.

This guide has so far only used narrow (`char`) ranges as input.
scnlib also supports wide (`wchar_t`) ranges to be used as source ranges,
including wide string literals and `std::wstring` s.
Wide strings are expected to be encoded in UTF-16 (with platform endianness), or UTF-32,
depending on the width of `wchar_t` (2 byte `wchar_t` -> UTF-16, 4 byte `wchar_t` -> UTF-32).

\code{.cpp}
auto result = scn::scan<std::wstring>(L"foo bar", L"{}");
// result->value() == L"foo"

// narrow strings can be scanned from wide sources, and vice versa
// in these cases, Unicode transcoding (UTF-8 <-> UTF-16/32) is performed
auto result2 = scn::scan<std::string>(result->range(), L"{}");
// result2->value() == "bar"
\endcode

\section usertypes User types

To scan a value of a user-defined type, specialize `scn::scanner`
with two member functions, `parse` and `scan`.

\code{.cpp}
struct mytype {
    int i;
    double d;
};

template <>
struct scn::scanner<mytype, char> {
    template <typename ParseContext>
    auto parse(ParseContext& pctx)
        -> scan_expected<typename ParseContext::iterator>;

    template <typename Context>
    auto scan(mytype& val, Context& ctx)
        -> scan_expected<typename Context::iterator>;
};
\endcode

`parse` parses the format string, and extracts scanning options from it.
The easiest ways to implement it are to inherit it from another type, or to just accept no options:

\code{.cpp}
// Inherit
template <>
struct scn::scanner<mytype, char> : scn::scanner<std::string_view, char> {};

// Accept only empty
template <typename ParseContext>
auto parse(ParseContext& pctx) -> scan_expected<typename ParseContext::iterator> {
    return pctx.begin();
}
\endcode

`scan` parses the actual value, using the supplied `Context`.
The context has a member function, `current`, to get an iterator pointing to the next character in the source range,
and `range`, to get the entire source range that's still left to scan.
These values can be then passed to `scn::scan`.
Alternatively, scanning can be delegated to another `scn::scanner`.

\code{.cpp}
template <typename Context>
auto scan(mytype& val, Context& ctx) -> scan_expected<typename Context::iterator> {
    auto result = scn::scan(ctx.range(), "{} {}");
    if (!result) {
        return unexpected(result.error());
    }

    val = {i, d};
    return result->begin();

    // or, delegate to other scanners (more advanced):

    return scn::scanner<int>{}.scan(val.i, ctx)
        .and_then([&](auto it) {
            ctx.advance_to(it);
            return scn::scanner<double>{}.scan(val.d, ctx);
        });
}
\endcode

If your type has an `std::istream` compatible `operator>>` overload, that can also be used for scanning.
Include the header `<scn/istream.h>`, and specialize `scn::scanner` by inheriting from `scn::istream_scanner`.

\code{.cpp}
std::istream& operator>>(std::istream&, const mytype&);

template <>
struct scn::scanner<mytype, char> : scn::istream_scanner {};
\endcode

\section locale Localization

By default, scnlib isn't affected by changes to the global C or C++ locale.
All functions behave as if the global locale were set to `"C"`.

A `std::locale` can be passed as the first argument to `scn::scan`, to scan using that locale.
This is mostly used with floats, to get locale-specific decimal separators.

Because of the way `std::locale` and the facilities around it work,
parsing using a locale is significantly slower compared to not using one.
This is, because the library effectively has to fall back on iostreams for parsing.

Just passing a locale isn't enough, but you'll need to opt-in to locale-specific parsing,
by using the `L` flag in the format string. Not every type supports localized parsing.

\code{.cpp}
auto result = scn::scan(std::locale{"fi_FI.UTF-8"}, "2,73", "{:L}");
// result->value() == 2.73
\endcode

Because localized scanning uses iostreams under the hood,
the results may not be entirely the same when no locale is used,
even if `std::locale::classic()` was passed.
This is due to limitations of the design of iostreams,
and platform-specific differences in locales and iostreams.