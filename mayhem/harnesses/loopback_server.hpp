// A one-connection-per-input loopback TCP server for the protocol-client harnesses: it accepts the
// client's connection, sends the fuzz bytes as the whole server side of the conversation, half-closes
// (so the client sees end-of-stream after the data), then drains whatever the client sends until the
// client hangs up. Plain POSIX sockets: this is test scaffolding, not the code under test.
#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <thread>

namespace loopback
{
inline int            g_listener = -1;
inline unsigned short g_port     = 0;

inline void init()
{
    g_listener = ::socket(AF_INET, SOCK_STREAM, 0);
    if (g_listener < 0)
        std::abort();
    const int one = 1;
    ::setsockopt(g_listener, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port        = 0; // ephemeral
    if (::bind(g_listener, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0 || ::listen(g_listener, 16) != 0)
        std::abort();
    socklen_t len = sizeof(addr);
    ::getsockname(g_listener, reinterpret_cast<sockaddr*>(&addr), &len);
    g_port = ntohs(addr.sin_port);
}

inline void serveOnce(const std::uint8_t* data, std::size_t size)
{
    const int client = ::accept(g_listener, nullptr, nullptr);
    if (client < 0)
        return;
    std::size_t sent = 0;
    while (sent < size)
    {
        const ssize_t n = ::send(client, data + sent, size - sent, MSG_NOSIGNAL);
        if (n <= 0)
            break;
        sent += static_cast<std::size_t>(n);
    }
    ::shutdown(client, SHUT_WR);
    char buffer[4096];
    while (::recv(client, buffer, sizeof(buffer), 0) > 0)
    {
    }
    ::close(client);
}

// Runs serveOnce on a thread while `client()` talks to 127.0.0.1:g_port.
template <typename Client>
void converse(const std::uint8_t* data, std::size_t size, Client&& client)
{
    std::thread server(serveOnce, data, size);
    client();
    server.join();
}
} // namespace loopback
