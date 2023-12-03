#include "socks4.hpp"


namespace socks4 { // threw in the kitchen sink for error codes
    namespace asio = boost::asio;
    using boost::system::error_category;
    using boost::system::error_condition;
    using boost::system::system_error;


    boost::system::error_category const& get_result_category() {
      struct impl : boost::system::error_category {
        const char* name() const noexcept override { return "socks4_result_code"; }
        std::string message(int ev) const override {
          switch (static_cast<result_code>(ev)) {
          case result_code::ok:                 return "Success";
          case result_code::invalid_version:    return "SOCKS4 invalid reply version";
          case result_code::rejected_or_failed: return "SOCKS4 rejected or failed";
          case result_code::need_identd:        return "SOCKS4 unreachable (client not running identd)";
          case result_code::unconfirmed_userid:  return "SOCKS4 identd could not confirm user ID";
          case result_code::failed:             return "SOCKS4 general unexpected failure";
          default: return "unknown error";
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
        bool equivalent(boost::system::error_code const& error,
                        int ev) const noexcept override {
            return error.value() == ev && &error.category() == this;
        }
      } const static instance;
      return instance;
    }

    boost::system::error_code make_error_code(result_code se) {
        return boost::system::error_code{
            static_cast<std::underlying_type<result_code>::type>(se),
            get_result_category()};
    }
} // namespace socks4
