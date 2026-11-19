// Fuzzes sf::Image::loadFromMemory (src/SFML/Graphics/Image.cpp with the bundled stb_image and qoi
// decoders: PNG, JPEG, BMP, TGA, GIF, PSD, HDR, PIC, PNM, QOI), then the image operations and the
// encoders in saveToMemory. No OpenGL context is needed.
#include <SFML/Graphics/Image.hpp>
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
    sf::Image image;
    if (!image.loadFromMemory(data, size))
        return 0;

    const sf::Vector2u dims = image.getSize();
    if (dims.x == 0 || dims.y == 0)
        return 0;
    (void)image.getPixel({dims.x - 1, dims.y - 1});

    // The post-processing below is linear in the pixel count; keep it to images a fuzzer can
    // afford every iteration (decoding itself is always exercised above).
    if (static_cast<std::uint64_t>(dims.x) * dims.y > (1u << 16))
        return 0;

    image.createMaskFromColor(image.getPixel({0, 0}));
    image.flipHorizontally();
    image.flipVertically();

    sf::Image copy({dims.x / 2 + 1, dims.y / 2 + 1});
    (void)copy.copy(image, {0, 0}, {{static_cast<int>(dims.x / 4), static_cast<int>(dims.y / 4)},
                                     {static_cast<int>(dims.x / 2), static_cast<int>(dims.y / 2)}},
                    true);

    static const char* const formats[] = {"png", "bmp", "tga", "jpg", "qoi"};
    (void)image.saveToMemory(formats[data[size / 2] % 5]);
    return 0;
}
