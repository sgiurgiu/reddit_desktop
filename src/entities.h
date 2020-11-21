#ifndef ENTITIES_H
#define ENTITIES_H

#include <string>
#include "json.hpp"

struct user
{
    user(){}
    user(const std::string& username,const std::string& password,
         const std::string& client_id,const std::string& secret,
         const std::string& website,const std::string& app_name):
        username(username),password(password),
        client_id(client_id),secret(secret),
        website(website),app_name(app_name)
    {}
    std::string username;
    std::string password;
    std::string client_id;
    std::string secret;
    std::string website;
    std::string app_name;
};

std::string make_user_agent(const user& u);

struct access_token
{
    std::string token;
    std::string tokenType;
    int expires;
    std::string scope;
};

struct listing
{
    nlohmann::json json;
};

template <typename T>
struct client_response
{
    T data;
    int status;
    std::string body;
};


#endif // ENTITIES_H
