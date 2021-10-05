#ifndef AWARDSRENDERER_H
#define AWARDSRENDERER_H


#include "entities.h"
#include "redditclientproducer.h"
#include <boost/asio/any_io_executor.hpp>
#include "utils.h"

class AwardsRenderer : public std::enable_shared_from_this<AwardsRenderer>
{
public:
    AwardsRenderer(post_ptr p);
    void Render();
    void LoadAwards(const access_token& token,
                    RedditClientProducer* client,
                    const boost::asio::any_io_executor& uiExecutor);
    static void ClearAwards();
private:
    static void loadAwardImage(std::string id,
                          Utils::STBImagePtr data, int width, int height,
                          int channels);
    void loadAwards(const access_token& token,
                    RedditClientProducer* client,
                    const boost::asio::any_io_executor& uiExecutor);
private:
    post_ptr awardedPost;
    std::vector<award> postAwards;
    std::string totalAwardsText;
   // access_token token;
    //RedditClientProducer* client;
    //const boost::asio::any_io_executor& uiExecutor;
};

#endif // AWARDSRENDERER_H
