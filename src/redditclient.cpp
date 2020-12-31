#include "redditclient.h"
#include <boost/beast/ssl.hpp>
#include "redditloginconnection.h"
#include <fmt/format.h>

#ifdef _WIN32
void add_windows_root_certs(boost::asio::ssl::context &ctx)
{
    HCERTSTORE hStore = CertOpenSystemStore(0, "ROOT");
    if (hStore == NULL) {
        return;
    }

    X509_STORE *store = X509_STORE_new();
    PCCERT_CONTEXT pContext = NULL;
    while ((pContext = CertEnumCertificatesInStore(hStore, pContext)) != NULL) {
        // convert from DER to internal format
        X509 *x509 = d2i_X509(NULL,
                              (const unsigned char **)&pContext->pbCertEncoded,
                              pContext->cbCertEncoded);
        if(x509 != NULL) {
            X509_STORE_add_cert(store, x509);
            X509_free(x509);
        }
    }

    CertFreeCertificateContext(pContext);
    CertCloseStore(hStore, 0);

    // attach X509_STORE to boost ssl context
    SSL_CTX_set_cert_store(ctx.native_handle(), store);
}
#endif

RedditClient::RedditClient(std::string_view authServer,std::string_view server, int clientThreadsCount):
    authServer(authServer),server(server),ssl_context(boost::asio::ssl::context::tls_client),
    work(boost::asio::make_work_guard(context))
{
    ssl_context.set_options(boost::asio::ssl::context::default_workarounds
                                //| boost::asio::ssl::context::no_sslv2
                                //| boost::asio::ssl::context::no_sslv3
                                );
    ssl_context.set_verify_mode(boost::asio::ssl::verify_peer);
    ssl_context.set_default_verify_paths();
    using run_function = boost::asio::io_context::count_type(boost::asio::io_context::*)();
    auto bound_run_fuction = std::bind(static_cast<run_function>(&boost::asio::io_context::run),std::ref(context));
    for (int i=0;i<clientThreadsCount;i++)
    {
        clientThreads.emplace_back(bound_run_fuction);
    }
}
RedditClient::~RedditClient()
{
    context.stop();
    for (auto&& thread:clientThreads)
    {
        if(thread.joinable())
        {
            thread.join();
        }
    }
}
void RedditClient::setUserAgent(const std::string& userAgent)
{
    this->userAgent = userAgent;
}
RedditClient::RedditLoginClientConnection RedditClient::makeLoginClientConnection()
{
    return std::make_shared<RedditLoginConnection>(context,ssl_context,authServer,service);
}
RedditClient::RedditListingClientConnection RedditClient::makeListingClientConnection()
{
    return std::make_shared<RedditListingConnection>(context,ssl_context,server,service,userAgent);
}
RedditClient::RedditResourceClientConnection RedditClient::makeResourceClientConnection()
{
    return std::make_shared<RedditResourceConnection>(context,ssl_context,userAgent);
}
RedditClient::MediaStreamingClientConnection RedditClient::makeMediaStreamingClientConnection()
{
    return std::make_shared<MediaStreamingConnection>(context,ssl_context,userAgent);
}
RedditClient::RedditCreatePostClientConnection RedditClient::makeCreatePostClientConnection()
{
    return std::make_shared<RedditCreatePostConnection>(context,ssl_context,server,service,userAgent);
}
RedditClient::RedditSearchNamesClientConnection RedditClient::makeRedditSearchNamesClientConnection()
{
    return std::make_shared<RedditSearchNamesConnection>(context,ssl_context,server,service,userAgent);
}
