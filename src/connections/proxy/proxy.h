#ifndef NETWORK_PROXY_H
#define NETWORK_PROXY_H

#include <string>
#include <optional>

namespace proxy {
constexpr auto PROXY_TYPE_SOCKS5 = "SOCKS5";
constexpr auto PROXY_TYPE_SOCKS4 = "SOCKS4";

struct proxy_t
{
    proxy_t(){}
    proxy_t(std::string host, std::string port,std::string username,std::string password, std::string proxyType, bool useProxy = false)
        : host{std::move(host)},port{std::move(port)},username{std::move(username)},
          password{std::move(password)},proxyType{std::move(proxyType)},useProxy{useProxy}
    {}
    std::string host;
    std::string port;
    std::string username;
    std::string password;
    std::string proxyType;
    bool useProxy = false;
};
}

bool operator==(const proxy::proxy_t& left,const proxy::proxy_t& right);
bool operator!=(const proxy::proxy_t& left,const proxy::proxy_t& right);

bool operator==(const std::optional<proxy::proxy_t>& left,const std::optional<proxy::proxy_t>& right);
bool operator!=(const std::optional<proxy::proxy_t>& left,const std::optional<proxy::proxy_t>& right);

#endif // PROXY_H
