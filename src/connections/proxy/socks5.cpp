#include "socks5.hpp"

namespace socks5 { // threw in the kitchen sink for error codes
    namespace asio = boost::asio;
    using boost::system::error_category;
    using boost::system::error_code;
    using boost::system::error_condition;
    using boost::system::system_error;

    boost::system::error_category const& get_result_category() {
      struct impl : boost::system::error_category {
        const char* name() const noexcept override { return "socks5_result_code"; }
        std::string message(int ev) const override {
          switch (static_cast<result_code>(ev)) {
          case result_code::ok:                         return "Success";
          case result_code::invalid_version:            return "SOCKS5 invalid reply version";
          case result_code::disallowed:                 return "SOCKS5 disallowed";
          case result_code::auth_method_rejected:       return "SOCKS5 no accepted authentication method";
          case result_code::network_unreachable:        return "SOCKS5 network unreachable";
          case result_code::host_unreachable:           return "SOCKS5 host unreachable";
          case result_code::connection_refused:         return "SOCKS5 connection refused";
          case result_code::ttl_expired:                return "SOCKS5 TTL expired";
          case result_code::command_not_supported:      return "SOCKS5 command not supported";
          case result_code::address_type_not_supported: return "SOCKS5 address type not supported";
          case result_code::failed:                     return "SOCKS5 general unexpected failure";
          default:                                      return "unknown error";
          }
        }
        error_condition
        default_error_condition(int ev) const noexcept override {
            return error_condition{ev, *this};
        }
        bool equivalent(int ev, error_condition const& condition)
            const noexcept override {
            return condition.value() == ev && &condition.category() == this;
        }
        bool equivalent(error_code const& error,
                        int ev) const noexcept override {
            return error.value() == ev && &error.category() == this;
        }
      } const static instance;
      return instance;
    }

    boost::system::error_code make_error_code(result_code se) {
        return error_code{
            static_cast<std::underlying_type<result_code>::type>(se),
            get_result_category()};
    }
} // namespace socks5
