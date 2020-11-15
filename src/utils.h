#ifndef UTILS_H
#define UTILS_H

#include <string>

class Utils
{
public:
    enum class Fonts: int
    {
        Roboto_Black = 1,
        Roboto_BlackItalic,
        Roboto_Bold,
        Roboto_BoldItalic,
        Roboto_Italic,
        Roboto_Light,
        Roboto_LightItalic,
        Roboto_Medium = 0,
        Roboto_MediumItalic,
        Roboto_Regular,
        Roboto_Thin,
        Roboto_ThinItalic
    };
    static int GetFontIndex(Fonts font);
    static void LoadFonts();
    static std::string convertSizeToHuman(uint64_t size);
    static std::string decode64(const std::string &val);
    static std::string encode64(const std::string &val);
private:
    static void AddFont(const unsigned int* fontData, const unsigned int fontDataSize, float fontSize);

};

#endif // UTILS_H
