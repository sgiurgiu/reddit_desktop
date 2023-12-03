#include "proxy.h"

bool operator==(const proxy::proxy_t& left,const proxy::proxy_t& right)
{
    return left.host == right.host && left.port == right.port &&
            left.username == right.username && left.password == right.password &&
            left.proxyType == right.proxyType && left.useProxy == right.useProxy;
}
bool operator!=(const proxy::proxy_t& left,const proxy::proxy_t& right)
{
    return !(left == right);
}
bool operator==(const std::optional<proxy::proxy_t>& left,const std::optional<proxy::proxy_t>& right)
{
    if(left && right)
    {
        return left.value() == right.value();
    }
    return false;
}
bool operator!=(const std::optional<proxy::proxy_t>& left,const std::optional<proxy::proxy_t>& right)
{
    return !(left == right);
}
