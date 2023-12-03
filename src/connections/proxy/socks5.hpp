#ifndef SOCKS5_PROXY_H
#define SOCKS5_PROXY_H


#include <boost/asio.hpp>
#include <boost/endian/arithmetic.hpp>

namespace socks5 {
enum class result_code {
    ok                         = 0,
    invalid_version            = 1,
    disallowed                 = 2,
    auth_method_rejected       = 3,
    network_unreachable        = 4,
    host_unreachable           = 5,
    connection_refused         = 6,
    ttl_expired                = 7,
    command_not_supported      = 8,
    address_type_not_supported = 9,
    //
    failed = 99,
};

boost::system::error_code make_error_code(result_code se);
boost::system::error_category const& get_result_category();
}

template <>
struct boost::system::is_error_code_enum<socks5::result_code>
    : std::true_type {};

namespace socks5 {
    using namespace std::placeholders;

    template <typename Proto> struct core_t {
        using Endpoint = typename Proto::endpoint;
        using Query    = typename boost::asio::ip::basic_resolver<Proto>::results_type;
        Endpoint _proxy;

        core_t(Query const& target, Endpoint proxy)
            : _proxy(proxy)
            , _request(target)
        {
        }
        core_t(Endpoint const& target, Endpoint proxy)
            : _proxy(proxy)
            , _request(target)
        {
        }

#pragma pack(push)
#pragma pack(1)
        enum class addr_type : uint8_t { IPv4 = 0x01, Domain = 0x03, IPv6 = 0x04 };
        enum class auth_method : uint8_t {
            none   = 0x00, // No authentication
            gssapi = 0x01, // GSSAPI (RFC 1961
            basic  = 0x02, // Username/password (RFC 1929)
            // 0x03–0x7F methods assigned by IANA[11]
            challenge_handshake = 0x03, // Challenge-Handshake Authentication Protocol
            challenge_response = 0x05,  // Challenge-Response Authentication Method
            ssl  = 0x06, // Secure Sockets Layer
            nds  = 0x07, // NDS Authentication
            maf  = 0x08, // Multi-Authentication Framework
            json = 0x09, // JSON Parameter Block
        };
        enum class version : uint8_t {
            none   = 0x00,
            socks4 = 0x04,
            socks5 = 0x05,
        };
        enum class proxy_command : uint8_t {
            connect       = 0x01,
            bind          = 0x02,
            udp_associate = 0x03,
        };
        enum class proxy_reply : uint8_t {
            succeeded                  = 0x00,
            general_failure            = 0x01,
            disallowed                 = 0x02,
            network_unreachable        = 0x03,
            host_unreachable           = 0x04,
            connection_refused         = 0x05,
            ttl_expired                = 0x06,
            command_not_supported      = 0x07,
            address_type_not_supported = 0x08,
        };

        using ipv4_octets = boost::asio::ip::address_v4::bytes_type;
        using ipv6_octets = boost::asio::ip::address_v6::bytes_type;
        using net_short   = boost::endian::big_uint16_t;

        struct {
            version     ver       = version::socks5;
            uint8_t     nmethods  = 0x01;
            auth_method method[1] = {auth_method::none};
        } _greeting;

        struct {
            version reply_version;
            uint8_t cauth;
        } _greeting_response;

        struct wire_address {
            wire_address(addr_type type): type{type}
            {}
            addr_type type{};
            union {
                ipv4_octets              ipv4;
                ipv6_octets              ipv6;
                std::array<uint8_t, 256> domain{0}; // length prefixed
            } payload;

            size_t var_length() const {
                return sizeof(type) + payload_length();
            }

            size_t payload_length() const
            {
                switch (type) {
                case addr_type::IPv4: return sizeof(payload.ipv4);
                case addr_type::IPv6: return sizeof(payload.ipv6);
                case addr_type::Domain:
                    assert(payload.domain[0] < payload.domain.max_size());
                    return 1 + payload.domain[0];
                }
                return 0;
            }
        };

        struct request_t {
            version       ver      = version::socks5;
            proxy_command cmd      = proxy_command::connect;
            uint8_t       reserved = 0;
            wire_address  var_address = {addr_type::IPv4};
            net_short     port;

            // constructors
            request_t(Endpoint const& ep) : port(ep.port())
            {
                auto addr = ep.address();
                if (addr.is_v4()) {
                    var_address.type         = addr_type::IPv4;
                    var_address.payload.ipv4 = addr.to_v4().to_bytes();
                } else {
                    var_address.type         = addr_type::IPv6;
                    var_address.payload.ipv6 = addr.to_v6().to_bytes();
                }
            }

            request_t(Query const& q) : port(std::stoi(q.service_name())) {
                std::string const domain = q.host_name();
                var_address.type         = addr_type::Domain;

                auto len = std::min(var_address.payload.domain.max_size() - 1,
                                    domain.length());
                assert(len == domain.length() || "domain truncated");
                var_address.payload.domain[0] = len;
                std::copy_n(domain.data(), len,
                            var_address.payload.domain.data() + 1);
            }

            auto buffers() const {
                return std::array {
                    boost::asio::buffer(this, offsetof(request_t, var_address)),
                    boost::asio::buffer(&var_address, var_address.var_length()),
                    boost::asio::buffer(&port, sizeof(port)),
                };
            }
        } _request;

        struct response_t {
            version      reply_version;
            proxy_reply  reply;
            uint8_t      reserved = 0x0;
            wire_address var_address = {addr_type::IPv4};
            net_short    port;

            auto head_buffers() {
                return std::array{
                    boost::asio::buffer(this, offsetof(response_t, var_address) + sizeof(addr_type)),
                };
            }

            auto tail_buffers() { // depends on head_buffers being correctly received!
                return std::array{
                    boost::asio::buffer(&var_address.payload, var_address.payload_length()),
                    boost::asio::buffer(&port, sizeof(port)),
                };
            }
        } _response;
#pragma pack(pop)

        using const_buffer   = boost::asio::const_buffer;
        using mutable_buffer = boost::asio::mutable_buffer;

        auto greeting_buffers() const {
            return boost::asio::buffer(&_greeting, sizeof(_greeting));
        }

        auto greeting_response_buffers() {
            return boost::asio::buffer(&_greeting_response, sizeof(_greeting_response));
        }

        auto request_buffers() const { return _request.buffers(); }
        auto response_head_buffers() { return _response.head_buffers(); }
        auto response_tail_buffers() { return _response.tail_buffers(); }

        boost::system::error_code get_greeting_result(boost::system::error_code ec = {}) const {
            if (ec)
                return ec;
            if (_greeting_response.reply_version != version::socks5)
                return result_code::invalid_version;

            if (_greeting_response.cauth != 0) {
                return result_code::auth_method_rejected;
            }

            return result_code::ok;
        }

        boost::system::error_code get_result(boost::system::error_code ec = {}) const {
            if (ec)
                return ec;
            if (_response.reply_version != version::socks5)
                return result_code::invalid_version;

            switch (_response.reply) {
            case proxy_reply::succeeded:                  return result_code::ok;
            case proxy_reply::disallowed:                 return result_code::disallowed;
            case proxy_reply::network_unreachable:        return result_code::network_unreachable;
            case proxy_reply::host_unreachable:           return result_code::host_unreachable;
            case proxy_reply::connection_refused:         return result_code::connection_refused;
            case proxy_reply::ttl_expired:                return result_code::ttl_expired;
            case proxy_reply::command_not_supported:      return result_code::command_not_supported;
            case proxy_reply::address_type_not_supported: return result_code::address_type_not_supported;
            case proxy_reply::general_failure: break;
            }
            return result_code::failed;
        };

    };

    template <typename Socket, typename Completion>
    struct async_proxy_connect_op {
        using Proto         = typename Socket::protocol_type;
        using Endpoint      = typename Proto::endpoint;
        using executor_type = typename Socket::executor_type;
        auto get_executor() { return _socket.get_executor(); }

      private:
        core_t<Proto> _core;
        Socket&       _socket;
        Completion    _handler;

      public:
        template <typename EndpointOrQuery>
        async_proxy_connect_op(Completion handler, Socket& s,
                               EndpointOrQuery target, Endpoint proxy)
            : _core(target, proxy)
            , _socket(s)
            , _handler(std::move(handler))
        {
        }

        using Self = std::unique_ptr<async_proxy_connect_op>;
        void init(Self&& self) { operator()(self, INIT{}); }

      private:
        // states
        struct INIT{};
        struct CONNECT{};
        struct GREETING_SENT{};
        struct ONGREETING_RESPONSE{};
        struct REQUEST_SENT{};
        struct ON_RESPONSE_HEAD{};
        struct ON_RESPONSE_TAIL{};

        struct Binder {
            Self _self;
            template <typename... Args>
            decltype(auto) operator()(Args&&... args) {
                return (*_self)(_self, std::forward<Args>(args)...);
            }
        };

        void operator()(Self& self, INIT) {
            _socket.async_connect(_core._proxy,
               std::bind(Binder{std::move(self)}, CONNECT{}, _1));
        }

        void operator()(Self& self, CONNECT, boost::system::error_code ec) {
            if (ec) return _handler(ec);
            boost::asio::async_write(
                _socket,
                _core.greeting_buffers(),
                std::bind(Binder{std::move(self)}, GREETING_SENT{}, _1, _2));
        }

        void operator()(Self& self, GREETING_SENT, boost::system::error_code ec, size_t xfer) {
            if (ec) return _handler(ec);
            auto buf = _core.greeting_response_buffers();
            boost::asio::async_read(
                _socket, buf, boost::asio::transfer_exactly(buffer_size(buf)),
                std::bind(Binder{std::move(self)}, ONGREETING_RESPONSE{}, _1, _2));
        }

        void operator()(Self& self, ONGREETING_RESPONSE, boost::system::error_code ec, size_t xfer) {
            ec = _core.get_greeting_result(ec);
            if (ec) return _handler(ec);

            boost::asio::async_write(
                _socket, _core.request_buffers(),
                std::bind(Binder{std::move(self)}, REQUEST_SENT{}, _1, _2));
        }

        void operator()(Self& self, REQUEST_SENT, boost::system::error_code ec, size_t xfer) {
            if (ec) return _handler(ec);
            auto buf = _core.response_head_buffers();
            boost::asio::async_read(
                _socket, buf, boost::asio::transfer_exactly(buffer_size(buf)),
                std::bind(Binder{std::move(self)}, ON_RESPONSE_HEAD{}, _1, _2));
        }

        void operator()(Self& self, ON_RESPONSE_HEAD, boost::system::error_code ec, size_t xfer) {
            if (ec) return _handler(ec);
            auto buf = _core.response_tail_buffers();
            boost::asio::async_read(
                _socket, buf, boost::asio::transfer_exactly(buffer_size(buf)),
                std::bind(Binder{std::move(self)}, ON_RESPONSE_TAIL{}, _1, _2));
        }

        void operator()(Self& self, ON_RESPONSE_TAIL, boost::system::error_code ec, size_t xfer) {
            _handler(_core.get_result(ec));
        }
    };

    template <typename Socket, typename EndpointOrQuery,
              typename Endpoint = typename Socket::protocol_type::endpoint,
              typename QueryResults = typename boost::asio::ip::basic_resolver<typename Socket::protocol_type>::results_type>
    boost::system::error_code proxy_connect(Socket& s, EndpointOrQuery target, QueryResults proxyQueryResults,
                             boost::system::error_code& ec)
    {        
        ec.clear();

        auto proxyEndpoint = s.connect(proxyQueryResults, ec);
        if(ec)
        {
            return ec;
        }
        core_t<typename Socket::protocol_type> core(target, proxyEndpoint);

        if (!ec)
            boost::asio::write(s, core.greeting_buffers(), ec);

        using boost::asio::transfer_exactly;
        if (!ec) {
            auto buf = core.greeting_response_buffers();
            boost::asio::read(s, buf, transfer_exactly(buffer_size(buf)), ec);
        }
        ec = core.get_greeting_result(ec);

        if (!ec) {
            boost::asio::write(s, core.request_buffers(), ec);
        }

        if (!ec) {
            auto buf = core.response_head_buffers();
            boost::asio::read(s, buf, transfer_exactly(buffer_size(buf)), ec);
        }
        if (!ec) {
            auto buf = core.response_tail_buffers();
            boost::asio::read(s, buf, transfer_exactly(buffer_size(buf)), ec);
        }

        return ec = core.get_result(ec);
    }

    template <typename Socket, typename EndpointOrQuery,
              typename Endpoint = typename Socket::protocol_type::endpoint>
    void proxy_connect(Socket& s, EndpointOrQuery target, Endpoint proxy)
    {
        boost::system::error_code ec;
        if (proxy_connect(s, target, proxy, ec))
            throw boost::system::system_error(ec);
    }

    template <typename Socket, typename Token, typename EndpointOrQuery,
              typename Endpoint = typename Socket::protocol_type::endpoint>
    auto async_proxy_connect(Socket& s, EndpointOrQuery target, Endpoint proxy,
                             Token&& token)
    {
        using Result = boost::asio::async_result<std::decay_t<Token>, void(boost::system::error_code)>;
        using Completion = typename Result::completion_handler_type;

        Completion completion(std::forward<Token>(token));
        Result     result(completion);

        using Op = async_proxy_connect_op<Socket, Completion>;
        // make an owning self ptr, to serve a unique async chain
        auto self = std::make_unique<Op>(completion, s, target, proxy);
        self->init(std::move(self));
        return result.get();
    }
} // namespace socks5

#endif
