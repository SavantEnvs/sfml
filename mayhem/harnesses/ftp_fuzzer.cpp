// Fuzzes the sf::Ftp client's reply parsing (src/SFML/Network/Ftp.cpp: the multi-line reply parser
// in getResponse, and DirectoryResponse's PWD path extraction) through the public API: an FTP
// session against a loopback server whose replies (all of them, in order) are the fuzz input.
// Directory listings / file transfers are not issued: they make the client connect to whatever
// address the PASV reply names.
#include "loopback_server.hpp"

#include <SFML/Network/Ftp.hpp>
#include <SFML/Network/IpAddress.hpp>
#include <SFML/System/Err.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
    sf::err().rdbuf(nullptr);
    loopback::init();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    loopback::converse(data, size, [&] {
        sf::Ftp ftp;
        if (!ftp.connect(sf::IpAddress::LocalHost, loopback::g_port).isOk())
            return;
        (void)ftp.login("anonymous", "fuzz@example.com").getMessage();
        const sf::Ftp::DirectoryResponse cwd = ftp.getWorkingDirectory();
        (void)cwd.getDirectory();
        (void)ftp.keepAlive().getStatus();
        (void)ftp.changeDirectory("pub").getMessage();
        (void)ftp.sendCommand("SYST").getMessage();
        (void)ftp.disconnect();
    });
    return 0;
}
