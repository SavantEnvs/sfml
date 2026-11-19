// Fuzzes the sf::Http client's response parsing (src/SFML/Network/Http.cpp: Http::Response::parse,
// parseFields and chunked transfer decoding), reached through the public API: sf::Http::sendRequest
// against a loopback server whose whole reply is the fuzz input.
#include "loopback_server.hpp"

#include <SFML/Network/Http.hpp>
#include <SFML/System/Err.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
    sf::err().rdbuf(nullptr);
    loopback::init();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    sf::Http           http("http://127.0.0.1", loopback::g_port);
    sf::Http::Response response;
    loopback::converse(data, size, [&] {
        sf::Http::Request request("/index.html", sf::Http::Request::Method::Get);
        request.setField("Accept", "*/*");
        response = http.sendRequest(request);
    });

    (void)response.getStatus();
    (void)response.getMajorHttpVersion();
    (void)response.getMinorHttpVersion();
    (void)response.getField("content-type");
    (void)response.getField("transfer-encoding");
    (void)response.getBody().size();
    return 0;
}
