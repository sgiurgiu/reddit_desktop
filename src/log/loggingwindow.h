#ifndef LOGGINGWINDOW_H
#define LOGGINGWINDOW_H

#include <boost/asio/any_io_executor.hpp>
#include <string>
#include <memory>
#include <vector>
#include "spdlogwindowsink.h"

class LoggingWindow : public std::enable_shared_from_this<LoggingWindow>
{
public:
    LoggingWindow(const boost::asio::any_io_executor& executor);
    ~LoggingWindow();
    bool isWindowOpen() const {return windowOpen;}
    void setWindowOpen(bool flag)
    {
        windowOpen = flag;
    }
    void showWindow(int appFrameWidth,int appFrameHeight);
    void setFocused();
    void setupLogging();
private:
    void addLogLines(std::vector<std::string> lines);
private:
    bool windowOpen = false;
    bool willBeFocused = false;
    bool scrollToBottom = false;
    const boost::asio::any_io_executor& uiExecutor;
    std::shared_ptr<SpdlogWindowSinkMt> logSink;
    std::vector<std::string> logLines;
};

#endif // LOGGINGWINDOW_H
