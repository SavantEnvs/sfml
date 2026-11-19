// Fuzzes sf::InputSoundFile::openFromMemory (src/SFML/Audio/InputSoundFile.cpp, SoundFileFactory,
// SoundFileReaderWav/Flac/Ogg/Mp3): sniffing the format, parsing the headers and decoding samples
// of WAV (SFML's own RIFF parser), FLAC, Ogg/Vorbis and MP3 files, then seeking and decoding again.
// No audio device is involved.
#include <SFML/Audio/InputSoundFile.hpp>
#include <SFML/System/Err.hpp>
#include <SFML/System/Time.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
    sf::err().rdbuf(nullptr); // decoders log every rejected file to sf::err()
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    sf::InputSoundFile file;
    if (!file.openFromMemory(data, size))
        return 0;

    const std::uint64_t count = file.getSampleCount();
    (void)file.getChannelCount();
    (void)file.getSampleRate();
    (void)file.getChannelMap();
    (void)file.getDuration();

    std::array<std::int16_t, 4096> samples{};
    for (int i = 0; i < 64; ++i)
        if (file.read(samples.data(), samples.size()) == 0)
            break;

    file.seek(count / 2);
    (void)file.read(samples.data(), samples.size());
    file.seek(0);
    (void)file.read(samples.data(), samples.size());
    return 0;
}
