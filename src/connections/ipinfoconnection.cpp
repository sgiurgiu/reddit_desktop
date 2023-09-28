#include "ipinfoconnection.h"

IPInfoConnection::IPInfoConnection(const boost::asio::any_io_executor& executor,
                                   boost::asio::ssl::context& ssl_context,
                                   const std::string& userAgent):
    RedditGetConnection<IpInfoResponse>{executor,ssl_context,"ipinfo.io","https"},userAgent{userAgent}
{
}

void IPInfoConnection::LoadIpInfo()
{
    request_t request;
    request.clear();
    request.version(11);
    request.method(boost::beast::http::verb::get);
    request.target("/");
    request.set(boost::beast::http::field::connection, "keep-alive");
    request.set(boost::beast::http::field::host, host);
    request.set(boost::beast::http::field::accept, "application/json");
    request.set(boost::beast::http::field::user_agent, userAgent);
    request.prepare_payload();

    performRequest(std::move(request));
}

void IPInfoConnection::responseReceivedComplete()
{
    auto status = responseParser->get().result_int();
    auto body = responseParser->get().body();
    IpInfoResponse response;
    response.status = status;
    boost::system::error_code ec = {};
    if(status == 200)
    {
        try
        {
            auto json = nlohmann::json::parse(body);

            if(json.contains("ip") && json["ip"].is_string())
            {
                response.data.ip = json["ip"].get<std::string>();
            }
            if(json.contains("hostname") && json["hostname"].is_string())
            {
                response.data.hostname = json["hostname"].get<std::string>();
            }
            if(json.contains("org") && json["org"].is_string())
            {
                response.data.org = json["org"].get<std::string>();
            }
            if(json.contains("city") && json["city"].is_string())
            {
                response.data.city = json["city"].get<std::string>();
            }
            if(json.contains("region") && json["region"].is_string())
            {
                response.data.region = json["region"].get<std::string>();
            }
            if(json.contains("country") && json["country"].is_string())
            {
                response.data.country = json["country"].get<std::string>();
            }
            if(json.contains("timezone") && json["timezone"].is_string())
            {
                response.data.timezone = json["timezone"].get<std::string>();
            }
        }
        catch(const std::exception& ex)
        {
            response.body = ex.what();
            if(status < 500) response.status = 500;
            cat.setMessage(body);
            ec.assign(response.status,cat);
        }
    }
    else
    {
        response.body = body;
        cat.setMessage(body);
        ec.assign(response.status,cat);
    }
    signal(ec,std::move(response));
}
