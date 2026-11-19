// Fuzzes sf::String and sf::Utf<8/16/32> (include/SFML/System/Utf.inl, src/SFML/System/String.cpp):
// decoding untrusted UTF-8/UTF-16/UTF-32 text, re-encoding it to every other encoding, the
// Unicode boundary queries and the find/replace/substring/erase/insert editing operations.
#include <SFML/System/String.hpp>
#include <SFML/System/Utf.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <string>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    if (size < 1)
        return 0;
    const std::uint8_t mode = data[0];
    const std::uint8_t* text = data + 1;
    const std::size_t   len  = size - 1;

    sf::String str;
    switch (mode % 3)
    {
        case 0:
            str = sf::String::fromUtf8(text, text + len, (mode & 0x80) ? std::optional<char32_t>(U'?') : std::nullopt);
            break;
        case 1:
        {
            std::vector<char16_t> u16(len / 2);
            if (!u16.empty())
                std::memcpy(u16.data(), text, u16.size() * 2);
            str = sf::String::fromUtf16(u16.begin(), u16.end(), (mode & 0x80) ? std::optional<char32_t>(U'?') : std::nullopt);
            break;
        }
        case 2:
        {
            std::vector<char32_t> u32(len / 4);
            if (!u32.empty())
                std::memcpy(u32.data(), text, u32.size() * 4);
            str = sf::String::fromUtf32(u32.begin(), u32.end());
            break;
        }
    }

    // Raw Utf8 helpers over the same bytes.
    (void)sf::Utf8::count(text, text + len);
    {
        std::u32string decoded;
        sf::Utf8::toUtf32(text, text + len, std::back_inserter(decoded));
        std::string latin1;
        sf::Utf8::toLatin1(text, text + len, std::back_inserter(latin1), '?');
    }

    // Re-encode to every encoding.
    const auto u8  = str.toUtf8();
    const auto u16 = str.toUtf16();
    const auto u32 = str.toUtf32();
    (void)str.toAnsiString();
    (void)str.toWideString();
    (void)str.toUtf8(std::uint8_t{'?'});
    (void)str.toUtf16(std::uint16_t{'?'});

    // Round trip of what we produced must be stable.
    const sf::String back = sf::String::fromUtf8(u8.begin(), u8.end());
    (void)(back == str);

    const std::size_t n = str.getSize();
    for (std::size_t i = 0; i <= n && i < 256; ++i)
    {
        (void)str.isGraphemeBoundary(i);
        (void)str.isWordBoundary(i);
        (void)str.isSentenceBoundary(i);
    }

    if (n > 0)
    {
        // pos < n, so the needle is never empty: String::replace(searchFor, ...)
        // with an empty searchFor matches at every position and never ends,
        // which is a caller error (same as any replace-all loop), not an SFML bug.
        const std::size_t pos = mode % n;
        const sf::String  needle = str.substring(pos, 3);
        (void)str.find(needle, n / 2);
        sf::String edited = str;
        edited.replace(needle, sf::String("xyz"));
        edited.insert(edited.getSize() / 2, needle);
        if (!edited.isEmpty())
            edited.erase(0, 1 + (mode % 4));
        edited.replace(0, edited.getSize() / 3, str);
        (void)edited.toUtf8();
    }
    return 0;
}
