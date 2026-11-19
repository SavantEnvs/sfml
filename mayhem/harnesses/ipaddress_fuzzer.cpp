// Fuzzes sf::IpAddress::fromString (src/SFML/Network/IpAddress.cpp): IPv4/IPv6 textual address
// parsing, as used for host names given to sf::Http, sf::Dns::resolve and user configuration, plus
// toString/toBytes and a round trip of the canonical form.
#include <SFML/Network/IpAddress.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    const std::string_view text(reinterpret_cast<const char*>(data), size);
    const auto address = sf::IpAddress::fromString(text);
    if (!address)
        return 0;

    const std::string canonical = address->toString();
    // toInteger()/toBytes() are documented as valid only for V4/V6 respectively (they assert).
    if (address->isV4())
        (void)address->toInteger();
    else
        (void)address->toBytes();

    // The canonical text form must parse back to the same address.
    const auto again = sf::IpAddress::fromString(canonical);
    if (!again || *again != *address)
        std::abort();
    return 0;
}
