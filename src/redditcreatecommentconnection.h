#ifndef REDDITCREATECOMMENTCONNECTION_H
#define REDDITCREATECOMMENTCONNECTION_H

#include "redditconnection.h"
#include "entities.h"

#include <boost/asio/ip/resolver_base.hpp>
#include <boost/beast/http.hpp>
#include <boost/signals2.hpp>
#include <optional>

class RedditCreateCommentConnection : public RedditConnection<
        boost::beast::http::string_body,
        boost::beast::http::string_body,
        const client_response<listing>&
    >
{
public:
    RedditCreateCommentConnection(const boost::asio::any_io_executor& executor,
                                  boost::asio::ssl::context& ssl_context,
                                  const std::string& host, const std::string& service,
                                  const std::string& userAgent);
    void createComment(const std::string& parentId,const std::string& text,
                    const access_token& token);
protected:
    virtual void responseReceivedComplete() override;
private:
    std::string userAgent;
};

#endif // REDDITCREATECOMMENTCONNECTION_H
