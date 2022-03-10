#ifndef URI_H
#define URI_H

#include <string>
#include <vector>

class Uri
{
public:
    Uri(const std::string& uri);
    bool isValid() const { return isValid_; }
    std::string scheme() const {return scheme_;}
    std::string host() const {return host_;}
    std::string port() const {return port_;}
    std::string query() const {return query_;}
    std::string fragment() const {return fragment_;}
    std::vector<std::string> path() const {return path_;}
    std::string fullPath(const std::string& delim = "/") const;
private:
    std::string uri_;
    std::string scheme_;
    std::string host_;
    std::string port_;
    std::string query_;
    std::string fragment_;
    std::vector<std::string> path_;
    bool isValid_ = false;
};

#endif // URI_H
