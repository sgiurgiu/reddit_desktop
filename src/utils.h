#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <memory>
#include "resizableglimage.h"
#include <stb_image.h>
#include "imgui.h"
#include "entities.h"

class Utils
{
public:
    enum class Fonts: int
    {
        Noto_Black = 0,
        Noto_BlackItalic,
        Noto_Bold,
        Noto_BoldItalic,
        Noto_Italic,
        Noto_Light,
        Noto_LightItalic,
        Noto_Medium,
        Noto_MediumItalic,
        Noto_Regular,
        Noto_Thin,
        Noto_ThinItalic,
        Noto_Medium_Big,
        Noto_MediumItalic_Big,
        NotoMono_Regular

        /*Roboto_Black = 0,
        Roboto_BlackItalic,
        Roboto_Bold,
        Roboto_BoldItalic,
        Roboto_Italic,
        Roboto_Light,
        Roboto_LightItalic,
        Roboto_Medium,
        Roboto_MediumItalic,
        Roboto_Regular,
        Roboto_Thin,
        Roboto_ThinItalic,
        Roboto_Medium_Big,
        Roboto_MediumItalic_Big,
        RobotoMono_Regular,
        RobotoMono_Medium,
        RobotoMono_Bold*/
    };
    static int GetFontIndex(Fonts font);
    static void LoadFonts();
    static std::string convertSizeToHuman(uint64_t size);
    static std::string decode64(const std::string &val);
    static std::string encode64(const std::string &val);
    static ResizableGLImagePtr loadImage(unsigned char* data, int width, int height, int channels);
    static ResizableGLImagePtr loadBlurredImage(unsigned char* data, int width, int height, int channels);
    static std::string getHumanReadableTimeAgo(uint64_t time);
    static std::string getHumanReadableNumber(int number);
    static stbi_uc * decodeImageData(stbi_uc const *buffer, int len, int *x, int *y, int *channels_in_file);
    static stbi_uc * decodeGifData(stbi_uc const *buffer, int len, int *x, int *y, int *channels_in_file,int *count, int** delays);
    static void openInBrowser(const std::string& url);
    static ImVec4 GetDownVoteColor();
    static ImVec4 GetUpVoteColor();
    static std::string CalculateScore(int& score,Voted originalVote,Voted newVote);
    static void LoadRedditThumbnails();
    static void ReleaseRedditThumbnails();
    static ResizableGLImagePtr GetRedditThumbnail(const std::string& kind);
    static std::string formatDuration(std::chrono::seconds diff);
private:
    static ImFont* AddFont(const unsigned int* fontData, const unsigned int fontDataSize, float fontSize);
private:
    static ResizableGLImagePtr redditThumbnails;    
};

#endif // UTILS_H
