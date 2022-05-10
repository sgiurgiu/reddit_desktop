#ifndef REDDITLOGINCONNECTION_H
#define REDDITLOGINCONNECTION_H

#include "redditconnection.h"
#include "entities.h"

#include <boost/system/error_code.hpp>
#include <boost/asio/ip/resolver_base.hpp>
#include <boost/beast/http.hpp>
#include <boost/signals2.hpp>

class RedditLoginConnection : public RedditPostConnection<client_response<access_token>>
{
public:
    RedditLoginConnection(const boost::asio::any_io_executor& executor,
                          boost::asio::ssl::context& ssl_context,const std::string& host,
                          const std::string& service);

    void login(const user& user);
protected:
    virtual void responseReceivedComplete() override;

};

class login_error_category : public boost::system::error_category
{
public:
    login_error_category(std::string errorMessage):
        errorMessage(std::move(errorMessage))
    {
    }
    login_error_category(){}
    const char *name() const noexcept
    {
        return "LoginErrorCategory";
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

#endif // REDDITLOGINCONNECTION_H
