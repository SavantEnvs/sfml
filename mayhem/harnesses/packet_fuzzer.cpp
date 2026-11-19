// Fuzzes sf::Packet deserialization (src/SFML/Network/Packet.cpp): the bytes a peer sends become the
// packet payload (Packet::append, as TcpSocket/UdpSocket::receive(Packet&) do), and the leading bytes
// choose the sequence of operator>> extractions a protocol would perform. Every extracted value is
// re-serialized into a second packet with operator<< and read back, as an application echoing
// fields would. Uses only API present in SFML 3.0 so it builds on older commits too.
#include <SFML/Network/Packet.hpp>
#include <SFML/System/String.hpp>

#include <cstddef>
#include <cstdint>
#include <string>

namespace
{
template <typename T>
void extract(sf::Packet& in, sf::Packet& out)
{
    T value{};
    if (in >> value)
        out << value;
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    if (size == 0)
        return 0;

    // data[0] = number of selector bytes that follow; the rest is the packet payload.
    const std::size_t nops = data[0] < size - 1 ? data[0] : size - 1;
    const std::uint8_t* ops = data + 1;

    sf::Packet in;
    in.append(data + 1 + nops, size - 1 - nops);
    sf::Packet out;

    for (std::size_t i = 0; i < nops && in; ++i)
    {
        switch (ops[i] % 14)
        {
            case 0: extract<bool>(in, out); break;
            case 1: extract<std::int8_t>(in, out); break;
            case 2: extract<std::uint8_t>(in, out); break;
            case 3: extract<std::int16_t>(in, out); break;
            case 4: extract<std::uint16_t>(in, out); break;
            case 5: extract<std::int32_t>(in, out); break;
            case 6: extract<std::uint32_t>(in, out); break;
            case 7: extract<std::int64_t>(in, out); break;
            case 8: extract<std::uint64_t>(in, out); break;
            case 9: extract<float>(in, out); break;
            case 10: extract<double>(in, out); break;
            case 11: extract<std::string>(in, out); break;
            case 12: extract<std::wstring>(in, out); break;
            case 13: extract<sf::String>(in, out); break;
        }
    }
    (void)in.endOfPacket();
    (void)in.getReadPosition();

    // Read the re-serialized packet back the same way.
    for (std::size_t i = 0; i < nops && out; ++i)
    {
        sf::Packet sink;
        switch (ops[i] % 14)
        {
            case 11: extract<std::string>(out, sink); break;
            case 12: extract<std::wstring>(out, sink); break;
            case 13: extract<sf::String>(out, sink); break;
            default: extract<std::uint8_t>(out, sink); break;
        }
    }
    return 0;
}
