#ifndef IPINFOCONNECTION_H
#define IPINFOCONNECTION_H

#include "redditconnection.h"
#include "entities.h"

struct IpInfo
{
    std::string ip;
    std::string hostname;
    std::string org;
    std::string city;
    std::string region;
    std::string country;
    std::string timezone;
};

using IpInfoResponse = client_response<IpInfo>;

class ipinfo_error_category : public boost::system::error_category
{
public:
    ipinfo_error_category(std::string errorMessage):
        errorMessage(std::move(errorMessage))
    {
    }
    ipinfo_error_category(){}
    const char *name() const noexcept
    {
        return "IpInfoErrorCategory";
    }
    std::string message(int) const
    {
        return errorMessage;
    }

    virtual bool failed( int ev ) const BOOST_NOEXCEPT
    {
        return ev != 200;
    }

    void setMessage(const std::string& msg)
    {
        errorMessage = msg;
    }
private:
    std::string errorMessage;
};


/**
 * @brief The IPInfoConnection class is a connection that retrieves ip information from
 * ipinfo.io
 * Yes, I know it's not a reddit connection, but it's a very convenient way to arrange the system.
 * One of these days I should rename the redditconnection to something more generic
 */
class IPInfoConnection : public RedditGetConnection<IpInfoResponse>
{
public:
    IPInfoConnection(const boost::asio::any_io_executor& executor,
                     boost::asio::ssl::context& ssl_context,
                     const std::string& userAgent);
    void LoadIpInfo();
protected:
    virtual void responseReceivedComplete() override;
private:
    std::string userAgent;
    ipinfo_error_category cat;
};

#endif // IPINFOCONNECTION_H
