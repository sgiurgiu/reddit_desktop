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
    AwardsRenderer(const comment& c);
    void Render();
    float RenderDirect(const ImVec2& pos);
    void LoadAwards(const access_token& token,
                    RedditClientProducer* client,
                    const boost::asio::any_io_executor& uiExecutor);

private:
    void loadAwards(const access_token& token,
                    RedditClientProducer* client,
                    const boost::asio::any_io_executor& uiExecutor);
private:
    int totalAwardsReceived = 0;
    std::vector<award> awards;
    std::string totalAwardsText;
    ImVec2 totalAwardsTextSize;
};

#endif // AWARDSRENDERER_H
