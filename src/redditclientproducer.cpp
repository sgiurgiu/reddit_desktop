#include "redditclientproducer.h"
#include <boost/beast/ssl.hpp>
#include <fmt/format.h>

#ifdef RD_WINDOWS
namespace
{
    #include <wincrypt.h>
    void add_windows_root_certs(boost::asio::ssl::context& ctx)
    {
        HCERTSTORE hStore = CertOpenSystemStoreA(0, "ROOT");
        if (hStore == NULL) {
            return;
        }

        X509_STORE* store = X509_STORE_new();
        PCCERT_CONTEXT pContext = NULL;
        while ((pContext = CertEnumCertificatesInStore(hStore, pContext)) != NULL) {
            // convert from DER to internal format
            X509* x509 = d2i_X509(NULL,
                (const unsigned char**)&pContext->pbCertEncoded,
                pContext->cbCertEncoded);
            if (x509 != NULL) {
                X509_STORE_add_cert(store, x509);
                X509_free(x509);
            }
        }

        CertFreeCertificateContext(pContext);
        CertCloseStore(hStore, 0);

        // attach X509_STORE to boost ssl context
        SSL_CTX_set_cert_store(ctx.native_handle(), store);
    }
}
#endif

RedditClientProducer::RedditClientProducer(std::string_view authServer,std::string_view server, int clientThreadsCount):
    authServer(std::move(authServer)),server(std::move(server)),ssl_context(boost::asio::ssl::context::tls_client),
    clientThreads(clientThreadsCount),executor(clientThreads.get_executor())
{
    ssl_context.set_options(boost::asio::ssl::context::default_workarounds
                                //| boost::asio::ssl::context::no_sslv2
                                //| boost::asio::ssl::context::no_sslv3
                                );
    ssl_context.set_verify_mode(boost::asio::ssl::verify_peer);
#ifdef RD_WINDOWS
    add_windows_root_certs(ssl_context);
#else
    ssl_context.set_default_verify_paths();
#endif
    
}
RedditClientProducer::~RedditClientProducer()
{
}
void RedditClientProducer::setUserAgent(const std::string& userAgent)
{
    this->userAgent = userAgent;
}
RedditClientProducer::RedditLoginClientConnection RedditClientProducer::makeLoginClientConnection()
{
    return std::make_shared<RedditLoginConnection>(executor,ssl_context,authServer,service);
}
RedditClientProducer::RedditListingClientConnection RedditClientProducer::makeListingClientConnection()
{
    return std::make_shared<RedditListingConnection>(executor,ssl_context,server,service,userAgent);
}
RedditClientProducer::RedditResourceClientConnection RedditClientProducer::makeResourceClientConnection()
{
    return std::make_shared<RedditResourceConnection>(executor,ssl_context,userAgent);
}
RedditClientProducer::UrlDetectionClientConnection RedditClientProducer::makeUrlDetectionClientConnection()
{
    return std::make_shared<UrlDetectionConnection>(executor,ssl_context,userAgent);
}
RedditClientProducer::RedditCreatePostClientConnection RedditClientProducer::makeCreatePostClientConnection()
{
    return std::make_shared<RedditCreatePostConnection>(executor,ssl_context,server,service,userAgent);
}
RedditClientProducer::RedditSearchNamesClientConnection RedditClientProducer::makeRedditSearchNamesClientConnection()
{
    return std::make_shared<RedditSearchNamesConnection>(executor,ssl_context,server,service,userAgent);
}
RedditClientProducer::RedditVoteClientConnection RedditClientProducer::makeRedditVoteClientConnection()
{
    return std::make_shared<RedditVoteConnection>(executor,ssl_context,server,service,userAgent);
}
RedditClientProducer::RedditMoreChildrenClientConnection RedditClientProducer::makeRedditMoreChildrenClientConnection()
{
    return std::make_shared<RedditMoreChildrenConnection>(executor,ssl_context,server,service,userAgent);
}
RedditClientProducer::RedditCreateCommentClientConnection RedditClientProducer::makeRedditCreateCommentClientConnection()
{
    return std::make_shared<RedditCreateCommentConnection>(executor,ssl_context,server,service,userAgent);
}
RedditClientProducer::RedditMarkReplyReadClientConnection RedditClientProducer::makeRedditMarkReplyReadClientConnection()
{
    return std::make_shared<RedditMarkReplyReadConnection>(executor,ssl_context,server,service,userAgent);
}
RedditClientProducer::RedditSRSubscriptionClientConnection RedditClientProducer::makeRedditRedditSRSubscriptionClientConnection()
{
    return std::make_shared<RedditSRSubscriptionConnection>(executor,ssl_context,server,service,userAgent);
}

RedditClientProducer::RedditLiveThreadClientConnection RedditClientProducer::makeRedditLiveThreadClientConnection()
{
    return std::make_shared<RedditLiveThreadConnection>(executor,ssl_context);
}
