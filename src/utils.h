#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <memory>
#include "resizableglimage.h"
#include <stb_image.h>
#include "imgui.h"
#include "entities.h"
#include <filesystem>

struct StandardRedditThumbnail
{
    StandardRedditThumbnail(ImVec2 uv0,ImVec2 uv1,ImVec2 size):
        uv0(std::move(uv0)),uv1(std::move(uv1)),size(std::move(size))
    {}
    ImVec2 uv0;
    ImVec2 uv1;
    ImVec2 size;
};

class Utils
{
public:
    enum class Fonts: int
    {
        Noto_Bold = 0,
        Noto_Italic,
        Noto_Light,
        Noto_LightItalic,
        Noto_Medium,
        Noto_Regular,
        Noto_Medium_Big,
        NotoMono_Regular
    };

    using STBImagePtr = std::shared_ptr<stbi_uc>;
    static int GetFontIndex(Fonts font);
    static void LoadFonts(const std::filesystem::path& executablePath);
    static void DeleteFonts();
    static std::string convertSizeToHuman(uint64_t size);
    static std::string decode64(const std::string &val);
    static std::string encode64(const std::string &val);
    static ResizableGLImagePtr loadImage(unsigned char* data, int width, int height, int channels);
    static ResizableGLImagePtr loadBlurredImage(unsigned char* data, int width, int height, int channels);
    static std::string getHumanReadableTimeAgo(uint64_t time, bool fuzzySeconds = false);
    static std::string getHumanReadableNumber(int number);
    static STBImagePtr decodeImageData(stbi_uc const *buffer, int len, int *x, int *y, int *channels_in_file);
    static STBImagePtr decodeGifData(stbi_uc const *buffer, int len, int *x, int *y, int *channels_in_file,int *count, int** delays);
    static void openInBrowser(const std::string& url);
    static ImVec4 GetDownVoteColor();
    static ImVec4 GetUpVoteColor();
    static std::string CalculateScore(int& score,Voted originalVote,Voted newVote);
    static void LoadRedditImages();
    static void ReleaseRedditImages();
    static std::optional<StandardRedditThumbnail> GetRedditThumbnail(const std::string& kind);
    static std::string formatDuration(std::chrono::seconds diff);
    static StandardRedditThumbnail GetRedditHeader();
    static ResizableGLImagePtr GetApplicationIcon();
    static ResizableGLImagePtr GetStrickenImage();
    static std::filesystem::path GetAppConfigFolder();
    static std::filesystem::path GetHomeFolder();
    static ResizableGLImageSharedPtr GetRedditDefaultSprites();
    static ImVec4 GetHTMLColor(const std::string& strColor);

private:
    static ImFont* AddFont(const std::filesystem::path& fontsFolder,
                           const std::string& font,
                           float fontSize);
    //static ResizableGLImagePtr GetRedditSpriteSubimage(int x, int y, int width, int height);
private:
    static ResizableGLImageSharedPtr redditDefaultSprites;
};

#endif // UTILS_H
