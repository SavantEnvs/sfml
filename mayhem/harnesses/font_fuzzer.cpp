// Fuzzes sf::Font::openFromMemory (src/SFML/Graphics/Font.cpp over FreeType): loading TrueType/
// OpenType/Type1/WOFF/... fonts, the font metrics and character map lookups.
// getGlyph and getKerning are not called: both load the glyph into a page whose sf::Texture needs an
// OpenGL context (an X display), which a headless fuzzing host does not have (SFML aborts in
// sf::priv::openDisplay).
#include <SFML/Graphics/Font.hpp>
#include <SFML/System/Err.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
    sf::err().rdbuf(nullptr);
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    sf::Font font;
    if (!font.openFromMemory(data, size))
        return 0;

    (void)font.getInfo();
    static const char32_t codePoints[] = {U'A', U'g', U'W', U'.', U' ', U'é', U'中', U'\U0001F600'};
    for (const unsigned int characterSize : {1u, 12u, 30u, 200u})
    {
        (void)font.getAscent(characterSize);
        (void)font.getDescent(characterSize);
        (void)font.getLineSpacing(characterSize);
        (void)font.getUnderlinePosition(characterSize);
        (void)font.getUnderlineThickness(characterSize);
        for (const char32_t a : codePoints)
            (void)font.hasGlyph(a);
    }
    return 0;
}
