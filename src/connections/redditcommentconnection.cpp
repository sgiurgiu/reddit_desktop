#include "redditcommentconnection.h"

#include <fmt/format.h>
#include "json.hpp"
#include <charconv>
#include "htmlparser.h"

RedditCommentConnection::RedditCommentConnection(const boost::asio::any_io_executor& executor,
                                                             boost::asio::ssl::context& ssl_context,
                                                             const std::string& host, const std::string& service,
                                                             const std::string& userAgent):
    RedditConnection(executor,ssl_context,host,service),userAgent(userAgent)
{

}
void RedditCommentConnection::deleteComment(const std::string& postId,
    const access_token& token, std::any userData)
{
    if (postId.empty()) return;
    request_t request;
    request.clear();
    request.version(11);
    request.method(boost::beast::http::verb::post);
    request.target("/api/del");
    request.set(boost::beast::http::field::host, host);
    request.set(boost::beast::http::field::accept, "*/*");
    request.set(boost::beast::http::field::connection, "keep-alive");
    request.set(boost::beast::http::field::user_agent, userAgent);
    request.set(boost::beast::http::field::authorization, fmt::format("Bearer {}", token.token));
    request.set(boost::beast::http::field::content_type, "application/x-www-form-urlencoded");
    request.body() = fmt::format("id={}&raw_json=1",postId);
    request.prepare_payload();
    enqueueRequest(std::move(request), std::move(userData));
}
void RedditCommentConnection::updateComment(const std::string& postId, const std::string& text,
    const access_token& token, std::any userData)
{
    if (text.empty() || postId.empty()) return;
    request_t request;
    request.clear();
    request.version(11);
    request.method(boost::beast::http::verb::post);
    request.target("/api/editusertext");
    request.set(boost::beast::http::field::host, host);
    request.set(boost::beast::http::field::accept, "*/*");
    request.set(boost::beast::http::field::connection, "keep-alive");
    request.set(boost::beast::http::field::user_agent, userAgent);
    request.set(boost::beast::http::field::authorization, fmt::format("Bearer {}", token.token));
    request.set(boost::beast::http::field::content_type, "application/x-www-form-urlencoded");
    auto escapedText = HtmlParser::escape(text);
    request.body() = fmt::format("api_type=json&text={}"
        "&thing_id={}&raw_json=1",
        escapedText, postId);
    request.prepare_payload();
    enqueueRequest(std::move(request), std::move(userData));
}
void RedditCommentConnection::createComment(const std::string& parentId,const std::string& text,
                                                  const access_token& token, std::any userData)
{
    if(text.empty() || parentId.empty()) return;
    request_t request;
    request.clear();
    request.version(11);
    request.method(boost::beast::http::verb::post);
    request.target("/api/comment");
    request.set(boost::beast::http::field::host, host);
    request.set(boost::beast::http::field::accept, "*/*");
    request.set(boost::beast::http::field::connection, "keep-alive");
    request.set(boost::beast::http::field::user_agent, userAgent);
    request.set(boost::beast::http::field::authorization,fmt::format("Bearer {}",token.token));
    request.set(boost::beast::http::field::content_type, "application/x-www-form-urlencoded");
    auto escapedText = HtmlParser::escape(text);
    request.body() = fmt::format("api_type=json&text={}"
                                 "&thing_id={}&raw_json=1",
                                 escapedText,parentId);
    request.prepare_payload();
    enqueueRequest(std::move(request), std::move(userData));
}

void RedditCommentConnection::responseReceivedComplete(std::any userData)
{
    auto status = responseParser->get().result_int();
    auto body = responseParser->get().body();
    client_response<listing> resp;
    for(const auto& h : responseParser->get())
    {
        if(h.name() == boost::beast::http::field::content_length)
        {
            auto val = h.value();
            std::from_chars(val.data(),val.data()+val.size(),resp.contentLength);
        }
        else if(h.name() == boost::beast::http::field::content_type)
        {
            resp.contentType = h.value();
        }
    }
    resp.status = status;
    if(status == 200)
    {
        try
        {
            resp.data.json = nlohmann::json::parse(body);
        }
        catch(const std::exception& ex)
        {
            resp.body = ex.what();
            if(status < 500) resp.status = 500;
        }
    }
    else
    {
        resp.body = body;
    }
    resp.userData = std::move(userData);
    signal({},std::move(resp));
}
