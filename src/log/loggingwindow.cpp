#include "loggingwindow.h"
#include <spdlog/spdlog.h>
#include <boost/asio/post.hpp>
#include <imgui.h>

LoggingWindow::LoggingWindow(const boost::asio::any_io_executor &executor):
    uiExecutor(executor)
{
}
void LoggingWindow::setupLogging()
{
    std::vector<spdlog::sink_ptr> sinks;
    logSink = std::make_shared<SpdlogWindowSinkMt>(uiExecutor);
    sinks.push_back(logSink);
    auto all_logger = std::make_shared<spdlog::logger>("log", sinks.begin(),sinks.end());
    all_logger->set_level(spdlog::level::trace);
    spdlog::register_logger(all_logger);
    spdlog::set_default_logger(all_logger);

    logSink->addLogSignal([weak=weak_from_this()](std::vector<std::string> lines){
        auto self = weak.lock();
        if(!self) return;
        boost::asio::post(self->uiExecutor,std::bind(&LoggingWindow::addLogLines,self, std::move(lines)));
    });
}
void LoggingWindow::showWindow(int appFrameWidth,int appFrameHeight)
{

    ImGui::SetNextWindowSize(ImVec2(appFrameWidth*0.6,appFrameHeight*0.8),ImGuiCond_FirstUseEver);
    if(!ImGui::Begin("Log Messages##_log_messages_window_",&windowOpen,ImGuiWindowFlags_None))
    {
        ImGui::End();
        return;
    }

    if(willBeFocused)
    {
        ImGui::SetWindowFocus();
        willBeFocused = false;
    }

    for(const auto& line : logLines)
    {
        ImGui::Selectable(line.c_str());
    }

    if(scrollToBottom)
    {
        ImGui::SetScrollHereY(1.f);
        scrollToBottom = false;
    }

    ImGui::End();
}
void LoggingWindow::setFocused()
{
    willBeFocused = true;
}
void LoggingWindow::addLogLines(std::vector<std::string> lines)
{
    std::move(lines.begin(),lines.end(),std::back_inserter(logLines));
    scrollToBottom = true;
}
