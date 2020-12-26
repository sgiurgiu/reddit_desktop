#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <memory>
#include "entities.h"
#include <stb_image.h>

class Utils
{
public:
    enum class Fonts: int
    {
        Roboto_Black = 0,
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
        RobotoMono_Bold
    };
    static int GetFontIndex(Fonts font);
    static void LoadFonts();
    static std::string convertSizeToHuman(uint64_t size);
    static std::string decode64(const std::string &val);
    static std::string encode64(const std::string &val);
    static gl_image_ptr loadImage(unsigned char* data, int width, int height, int channels);
    static gl_image_ptr loadBlurredImage(unsigned char* data, int width, int height, int channels);
    static std::string getHumanReadableTimeAgo(uint64_t time);
    static std::string getHumanReadableNumber(int number);
    static stbi_uc * decodeImageData(stbi_uc const *buffer, int len, int *x, int *y, int *channels_in_file);
    static stbi_uc * decodeGifData(stbi_uc const *buffer, int len, int *x, int *y, int *channels_in_file,int *count, int** delays);
private:
    static void AddFont(const unsigned int* fontData, const unsigned int fontDataSize, float fontSize);

};

#endif // UTILS_H
