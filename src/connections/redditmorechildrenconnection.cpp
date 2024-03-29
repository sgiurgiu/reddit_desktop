#include "redditmorechildrenconnection.h"
#include <boost/beast/core.hpp>
#include <boost/system/error_code.hpp>
#include <fmt/format.h>
#include "json.hpp"
#include <charconv>

#include "utils.h"

RedditMoreChildrenConnection::RedditMoreChildrenConnection(const boost::asio::any_io_executor& executor,
                                                           boost::asio::ssl::context& ssl_context,
                                                           const std::string& host, const std::string& service,
                                                           const std::string& userAgent):
    RedditConnection(executor,ssl_context,host,service),userAgent(userAgent)
{

}

void RedditMoreChildrenConnection::list(const unloaded_children& children,
                                        const std::string& linkId,
                                        const access_token& token, std::any userData)
{
    std::string commaSeparator;
    std::string childrensList;
    for(const auto& child:children.children)
    {
        childrensList+=std::exchange(commaSeparator,"%2C")+child;
    }

    std::string body = "link_id="+linkId+
                        "&sort=confidence&limit_children=false&raw_json=1&api_type=json"+
                        "&children="+childrensList;
    request_t request;

    request.version(11);
    request.method(boost::beast::http::verb::post);
    request.target("/api/morechildren");
    request.set(boost::beast::http::field::connection, "keep-alive");
    request.set(boost::beast::http::field::host, host);
    request.set(boost::beast::http::field::accept, "application/json");
    request.set(boost::beast::http::field::user_agent, userAgent);
    request.set(boost::beast::http::field::content_type, "application/x-www-form-urlencoded");
    request.set(boost::beast::http::field::authorization,fmt::format("Bearer {}",token.token));
    request.body() = body;
    request.prepare_payload();
    responseParser->get().body().clear();
    responseParser->get().clear();
    enqueueRequest(std::move(request),std::move(userData));
}

void RedditMoreChildrenConnection::responseReceivedComplete(std::any userData)
{
    auto status = responseParser->get().result_int();
    auto body = responseParser->get().body();
    client_response<listing> resp;
    fillResponseHeaders(resp);
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
