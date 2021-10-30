#ifndef FLAIRRENDERER_H
#define FLAIRRENDERER_H

#include "entities.h"
#include "redditclientproducer.h"
#include <boost/asio/any_io_executor.hpp>
#include "utils.h"
#include <memory>
#include <vector>

class FlairRenderer : public std::enable_shared_from_this<FlairRenderer>
{
public:
    FlairRenderer(post_ptr p);
    void Render();
    void LoadFlair(const access_token& token,
                    RedditClientProducer* client,
                    const boost::asio::any_io_executor& uiExecutor);

private:

    class flair
    {
    public:
        template<typename T>
        flair(T t) : self(std::make_shared<flair_model<T>>(std::move(t)))
        {}
        friend void render(const flair& f)
        {
            f.self->render_();
        }
        friend void retrieve_data(const flair& f,
                                  const access_token& token,
                                  RedditClientProducer* client,
                                  const boost::asio::any_io_executor& uiExecutor)
        {
            f.self->retrieve_data_(token,client,uiExecutor);
        }
    private:
        struct flair_concept {
            virtual ~flair_concept() = default;
            virtual void render_() const = 0;
            virtual void retrieve_data_(const access_token& token,
                                        RedditClientProducer* client,
                                        const boost::asio::any_io_executor& uiExecutor) const = 0;
        };

        template<typename T>
        struct flair_model : flair_concept {
            flair_model(T t) : data(std::move(t)) {}
            void render_() const override;
            void retrieve_data_(const access_token& token,
                                RedditClientProducer* client,
                                const boost::asio::any_io_executor& uiExecutor) const override;
            T data;
        };
        std::shared_ptr<const flair_concept> self;
    };


    void loadFlair(const access_token& token,
                    RedditClientProducer* client,
                    const boost::asio::any_io_executor& uiExecutor);

    using flairs_t = std::vector<flair>;
    flairs_t flairs;
};

#endif // FLAIRRENDERER_H
