#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <glad/glad.h>
#include <memory>
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
    };
    struct gl_image
    {
        GLuint textureId = 0;
        int width = 0;
        int height = 0;
        int channels = 0;
        ~gl_image();
    };
    static int GetFontIndex(Fonts font);
    static void LoadFonts();
    static std::string convertSizeToHuman(uint64_t size);
    static std::string decode64(const std::string &val);
    static std::string encode64(const std::string &val);
    static std::unique_ptr<gl_image> LoadImage(unsigned char* data, int width, int height, int channels);
private:
    static void AddFont(const unsigned int* fontData, const unsigned int fontDataSize, float fontSize);

};

#endif // UTILS_H
