#include "utils.h"
#include "macros.h"


#include <array>
#include <fmt/format.h>
#include <chrono>
#include <iostream>
#include <locale>
#include <codecvt>

#ifdef RD_WINDOWS
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <objbase.h>
#else
#include <boost/process/spawn.hpp>
#include <boost/process/search_path.hpp>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvla"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4100 ) //TODO: find out warning numbers of MSVC
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define IIR_GAUSS_BLUR_IMPLEMENTATION
#include "iir_gauss_blur.h"

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

namespace {
    std::map<std::string, std::tuple<float,float,float,float>> thumbnailsCoordinates = {
        {"image",   {0.f,310.f          ,140.f,105.f}},
        {"default", {0.f,310.f+(148.f)  ,140.f,105.f}},
        {"nsfw",    {0.f,310.f+(290.f)  ,140.f,105.f}},
        {"spoiler", {0.f,310.f+(440.f)  ,140.f,105.f}},
        {"self",    {0.f,310.f+(585.f)  ,140.f,105.f}},
        {"reddit",  {0.f,310.f+(445.f)  ,140.f,105.f}}, //same as "spoiler"
    };

    struct STBImageDeleter
    {
        void operator()(stbi_uc *data)
        {
            if(data)
            {
                stbi_image_free(data);
            }
        }
    };


}



std::string Utils::convertSizeToHuman(uint64_t size)
{
    const std::array<std::string,6> suffix = {"B", "KB", "MB", "GB", "TB"};
    const auto length = suffix.size();
    size_t i = 0;
    double dblBytes = static_cast<double>(size);

    if (size >= 1024) {
        for (i = 0; (size / 1024) > 0 && i<length-1; i++, size /= 1024)
            dblBytes = size / 1024.0;
    }

    if(size == 0)
    {

    }

    auto xx = fmt::format("{0:.0f} {1:s}",dblBytes, suffix[i]);
    return xx;
}

Utils::STBImagePtr Utils::decodeImageData(stbi_uc const *buffer, int len, int *x, int *y, int *channels_in_file)
{
    return STBImagePtr(stbi_load_from_memory(buffer,len,x,y,channels_in_file,STBI_rgb_alpha),STBImageDeleter());
}
Utils::STBImagePtr Utils::decodeGifData(stbi_uc const *buffer, int len, int *x, int *y,
                               int *channels_in_file, int *count, int** delays)
{
    return STBImagePtr(stbi_load_gif_from_memory(buffer, len, delays, x, y, count, channels_in_file, STBI_rgb_alpha),STBImageDeleter());
}

std::string Utils::getHumanReadableNumber(int number)
{
    std::string fmt_num;
    if(number < 10000 )
    {
        fmt_num = fmt::format("{}",number);
    }
    else if(number < 100'000 )
    {
        fmt_num = fmt::format("{:.1f}k",number/1000.0);
    }
    else
    {
        fmt_num = fmt::format("{:.0f}k",number/1000.0);
    }
    return fmt_num;
}
std::string Utils::getHumanReadableTimeAgo(uint64_t time, bool fuzzySeconds)
{
    auto diff_time = std::time(nullptr) - time;
    std::chrono::seconds diff(diff_time);
    typedef std::chrono::duration<int, std::ratio<86400*365>> years;
    const auto y = std::chrono::duration_cast<years>(diff);
    if(y.count() > 0)
    {
        if(y.count() == 1) return "a year ago";
        return fmt::format("{} years ago", y.count());
    }
    typedef std::chrono::duration<int, std::ratio<86400>> days;
    const auto d = std::chrono::duration_cast<days>(diff);
    if(d.count() > 0)
    {
        if(d.count() == 1) return "yesterday";
        return fmt::format("{} days ago", d.count());
    }
    const auto h = std::chrono::duration_cast<std::chrono::hours>(diff);
    if(h.count() > 0)
    {
        if(h.count() == 1) return "1 hour ago";
        return fmt::format("{} hours ago", h.count());
    }
    const auto m = std::chrono::duration_cast<std::chrono::minutes>(diff);
    if(m.count() > 0)
    {
        if(m.count() == 1) return "1 minute ago";
        return fmt::format("{} minutes ago", m.count());
    }
    if(fuzzySeconds)
    {
        return "just now";
    }
    const auto s = std::chrono::duration_cast<std::chrono::seconds>(diff);
    if(s.count() > 0)
    {
        if(s.count() == 1) return "1 second ago";
        return fmt::format("{} seconds ago", s.count());
    }
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
    return fmt::format("{} miliseconds ago", ms.count());
}
std::string Utils::formatDuration(std::chrono::seconds diff)
{
    std::string text;
    const auto h = std::chrono::duration_cast<std::chrono::hours>(diff);
    auto hours_count = h.count();
    if(hours_count > 0)
    {
        if(hours_count < 10) text="0";
        text+= std::to_string(hours_count) + "h:";
    }
    diff = diff - h;
    const auto m = std::chrono::duration_cast<std::chrono::minutes>(diff);
    auto min_count = m.count();
    if(hours_count > 0 || min_count > 0)
    {
        if(min_count < 10) text+="0";
        text += std::to_string(min_count) + "m:";
    }
    diff = diff - m;
    auto sec_count = diff.count();
    if(sec_count < 10) text+="0";
    text += std::to_string(sec_count)+"s";
    return text;
}
void Utils::openInBrowser(const std::string& url)
{

#ifdef RD_WINDOWS    
    SHELLEXECUTEINFO ShExecInfo;
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask = NULL;
    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = NULL;
    ShExecInfo.lpFile = url.c_str();
    ShExecInfo.lpParameters = NULL;
    ShExecInfo.lpDirectory = NULL;
    ShExecInfo.nShow = SW_MAXIMIZE;
    ShExecInfo.hInstApp = NULL;

    ShellExecuteEx(&ShExecInfo);
    //ShellExecute(NULL, L"open", url.c_str(), NULL, NULL, 0);
#else
    auto browser = boost::process::search_path("xdg-open");
    if (browser.empty())
    {
        std::cerr << "Cannot find xdg-open in PATH" << std::endl;
        return;
    }
    boost::process::spawn(browser, url);
#endif
}
std::string Utils::CalculateScore(int& score,Voted originalVote,Voted newVote)
{
    if(originalVote != newVote)
    {
        switch(newVote)
        {
        case Voted::DownVoted:
            score -= (originalVote == Voted::UpVoted) ? 2 : 1;
            break;
        case Voted::UpVoted:
            score += (originalVote == Voted::DownVoted) ? 2 : 1;
            break;
        case Voted::NotVoted:
            score += (originalVote == Voted::DownVoted) ? 1 : -1;
            break;
        }
    }
    return Utils::getHumanReadableNumber(score);
}

std::filesystem::path Utils::GetAppConfigFolder()
{
    std::filesystem::path homePath;
    std::string relativeConfigFolder;
#ifdef RD_WINDOWS
    char homeDirStr[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, homeDirStr)))
    {
        homePath = homeDirStr;
    }
    relativeConfigFolder = "reddit_desktop";
#else
    auto pwd = getpwuid(getuid());
    if (pwd)
    {
        homePath = pwd->pw_dir;
    }
    else
    {
        // try the $HOME environment variable
        homePath = getenv("HOME");
    }
    relativeConfigFolder = ".config/reddit_desktop";
#endif


    if(homePath.empty())
    {
        homePath = "./";
    }

    std::filesystem::path  configFolder = homePath / relativeConfigFolder;
    std::filesystem::create_directories(configFolder);
    return configFolder;
}

