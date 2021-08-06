#ifndef SPDLOGWINDOWSINK_H
#define SPDLOGWINDOWSINK_H

#include <spdlog/sinks/base_sink.h>
#include <spdlog/common.h>
#include <spdlog/details/null_mutex.h>
#include <mutex>
#include <boost/asio/any_io_executor.hpp>
#include <boost/signals2.hpp>
#include <boost/asio/steady_timer.hpp>
#include <vector>
#include <chrono>

template <typename Mutex>
class SpdlogWindowSink final : public spdlog::sinks::base_sink<Mutex>,
        public std::enable_shared_from_this<SpdlogWindowSink<Mutex>>
{
public:
    SpdlogWindowSink(const boost::asio::any_io_executor &executor):
        logsPostingTimer(executor)
    {}
    ~SpdlogWindowSink() override = default;
    SpdlogWindowSink(const SpdlogWindowSink &) = delete;
    SpdlogWindowSink &operator=(const SpdlogWindowSink &) = delete;
    template<typename S>
    void addLogSignal(S slot)
    {
        logSignal.connect(slot);
    }
    void cancel()
    {
        std::lock_guard<Mutex> _(mutex);
        lines.clear();
        logsPostingTimer.cancel();
        logSignal.disconnect_all_slots();
        cancelled = true;
    }
protected:
    void sink_it_(const spdlog::details::log_msg &msg) override
    {
        std::lock_guard<Mutex> _(mutex);
        if (cancelled) return;
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
        lines.emplace_back(formatted.data(), static_cast<std::streamsize>(formatted.size()));        
        logsPostingTimer.expires_after(std::chrono::seconds(1));
        logsPostingTimer.async_wait([weak = this->weak_from_this()](const boost::system::error_code& ec){
            auto self = weak.lock();
            if(self && !ec)
            {
                std::lock_guard<Mutex> _(self->mutex);
                if (self->cancelled) return;
                std::vector<std::string> tmp;
                std::move(self->lines.begin(),self->lines.end(),std::back_inserter(tmp));
                self->lines.clear();
                self->logSignal(std::move(tmp));
            }
        });
    }

    void flush_() override
    {
    }
private:
    using LogSignal = boost::signals2::signal<void(std::vector<std::string>)>;
    boost::asio::steady_timer logsPostingTimer;
    LogSignal logSignal;
    Mutex mutex;
    std::vector<std::string> lines;
    bool cancelled = false;
};

using SpdlogWindowSinkMt = SpdlogWindowSink<std::mutex>;
using SpdlogWindowSinkSt = SpdlogWindowSink<spdlog::details::null_mutex>;

#endif // SPDLOGWINDOWSINK_H
