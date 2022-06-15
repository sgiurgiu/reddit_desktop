#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <memory>
#include <stb_image.h>
#include "entities.h"
#include <filesystem>


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
    static std::string getHumanReadableTimeAgo(uint64_t time, bool fuzzySeconds = false);
    static std::string getHumanReadableNumber(int number);
    static STBImagePtr decodeImageData(stbi_uc const *buffer, int len, int *x, int *y, int *channels_in_file);
    static STBImagePtr decodeGifData(stbi_uc const *buffer, int len, int *x, int *y, int *channels_in_file,int *count, int** delays);
    static void openInBrowser(const std::string& url);
    static std::string CalculateScore(int& score,Voted originalVote,Voted newVote);
    static void LoadRedditImages();
    static void ReleaseRedditImages();

    static std::string formatDuration(std::chrono::seconds diff);
    static std::filesystem::path GetAppConfigFolder();
    //static ResizableGLImagePtr GetRedditSpriteSubimage(int x, int y, int width, int height);
};

#endif // UTILS_H
