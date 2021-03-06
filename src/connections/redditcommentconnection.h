#ifndef REDDITCOMMENTCONNECTION_H
#define REDDITCOMMENTCONNECTION_H

#include "redditconnection.h"
#include "entities.h"

#include <boost/beast/http.hpp>


class RedditCommentConnection : public RedditPostConnection<client_response<listing>>
{
public:
    RedditCommentConnection(const boost::asio::any_io_executor& executor,
                                  boost::asio::ssl::context& ssl_context,
                                  const std::string& host, const std::string& service,
                                  const std::string& userAgent);
    void createComment(const std::string& parentId,const std::string& text,
                    const access_token& token, std::any userData = std::any());
    void updateComment(const std::string& postId, const std::string& text,
        const access_token& token, std::any userData = std::any());
    void deleteComment(const std::string& postId, const access_token& token,
                       std::any userData = std::any());
protected:
    virtual void responseReceivedComplete(std::any userData) override;
private:
    std::string userAgent;
};

#endif // REDDITCREATECOMMENTCONNECTION_H
