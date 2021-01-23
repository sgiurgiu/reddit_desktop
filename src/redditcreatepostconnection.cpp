#include "redditcreatepostconnection.h"
#include <fmt/format.h>
#include "json.hpp"
#include <charconv>
#include "htmlparser.h"

RedditCreatePostConnection::RedditCreatePostConnection(const boost::asio::any_io_executor& executor,
                                                       boost::asio::ssl::context& ssl_context,
                                                       const std::string& host, const std::string& service,
                                                       const std::string& userAgent):
    RedditConnection(executor,ssl_context,host,service),userAgent(userAgent)
{
}

void RedditCreatePostConnection::createPost(const post_ptr& p,bool sendReplies,
                                            const access_token& token)
{
    request_t request;
    request.version(11);
    request.method(boost::beast::http::verb::post);
    request.target("/api/submit");
    request.set(boost::beast::http::field::host, host);
    request.set(boost::beast::http::field::accept, "*/*");
    request.set(boost::beast::http::field::connection, "keep-alive");
    request.set(boost::beast::http::field::user_agent, userAgent);
    request.set(boost::beast::http::field::authorization,fmt::format("Bearer {}",token.token));
    request.set(boost::beast::http::field::content_type, "application/x-www-form-urlencoded");
    auto title = HtmlParser::escape(p->title);
    if(!p->selfText.empty())
    {
        auto text = HtmlParser::escape(p->selfText);
        request.body() = fmt::format("kind={}&api_type=json&text={}"
                                     "&title={}&sr={}&sendreplies={}&nsfw={}&resubmit=true",
                                     p->postHint,text,title,p->subreddit,sendReplies,p->over18);
    }
    if(!p->url.empty())
    {
        auto url = HtmlParser::escape(p->url);
        request.body() = fmt::format("kind={}&api_type=json&url={}"
                                     "&title={}&sr={}&sendreplies={}&nsfw={}&resubmit=true",
                                     p->postHint,url,title,p->subreddit,sendReplies,p->over18);
    }

    this->p = p;
    request.prepare_payload();
    responseParser->get().body().clear();
    responseParser->get().clear();
    performRequest(std::move(request));
}

void RedditCreatePostConnection::responseReceivedComplete()
{
    auto status = responseParser->get().result_int();
    auto body = responseParser->get().body();
    client_response<post_ptr> resp;
    for(const auto& h : responseParser->get())
    {
        if(h.name() == boost::beast::http::field::content_length)
        {
            auto val = h.value();
            std::from_chars(val.data(),val.data()+val.size(),resp.contentLength);
        }
        else if(h.name() == boost::beast::http::field::content_type)
        {
            resp.contentType = h.value().to_string();
        }
    }
    resp.status = status;
    if(status == 200)
    {
        try
        {
            auto json = nlohmann::json::parse(body);
            if(json.contains("success") && json["success"].is_boolean())
            {
                bool success = json["success"].get<bool>();
                if(!success)
                {
                    //not fine

                }
            }
            else if(json.contains("json") && json["json"].is_object())
            {
                const auto& respJson = json["json"];
                if(respJson.contains("data") && respJson["data"].is_object() &&
                        respJson["data"].contains("id") && respJson["data"]["id"].is_string())
                {
                    p->id = respJson["data"]["id"].get<std::string>();
                    resp.data = p;
                }
            }
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
    signal({},resp);
}
