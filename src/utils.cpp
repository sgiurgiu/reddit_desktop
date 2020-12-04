#include "utils.h"
#include "imgui.h"
#include "fonts/fonts.h"
#include <array>
#include <fmt/format.h>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>
#include <chrono>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

void Utils::AddFont(const unsigned int* fontData, const unsigned int fontDataSize, float fontSize)
{
    ImFontConfig config;
    config.MergeMode = true;
    config.GlyphMinAdvanceX = fontSize; // Use if you want to make the icon monospaced
    static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

    ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(fontData,fontDataSize, fontSize);
    ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(FontAwesome_compressed_data, FontAwesome_compressed_size, fontSize,
                                                               &config, icon_ranges);

}

void Utils::LoadFonts()
{
    AddFont(Roboto_Black_ttf_compressed_data,Roboto_Black_ttf_compressed_size,16.0f);
    AddFont(Roboto_BlackItalic_ttf_compressed_data,Roboto_BlackItalic_ttf_compressed_size,16.0f);
    AddFont(Roboto_Bold_ttf_compressed_data,Roboto_Bold_ttf_compressed_size,16.0f);
    AddFont(Roboto_BoldItalic_ttf_compressed_data,Roboto_BoldItalic_ttf_compressed_size,16.0f);
    AddFont(Roboto_Italic_ttf_compressed_data,Roboto_Italic_ttf_compressed_size,16.0f);
    AddFont(Roboto_Light_ttf_compressed_data,Roboto_Light_ttf_compressed_size,16.0f);
    AddFont(Roboto_LightItalic_ttf_compressed_data,Roboto_LightItalic_ttf_compressed_size,16.0f);
    AddFont(Roboto_Medium_ttf_compressed_data,Roboto_Medium_ttf_compressed_size, 16.0f);
    AddFont(Roboto_MediumItalic_ttf_compressed_data,Roboto_MediumItalic_ttf_compressed_size, 16.0f);
    AddFont(Roboto_Regular_ttf_compressed_data,Roboto_Regular_ttf_compressed_size,16.0f);
    AddFont(Roboto_Thin_ttf_compressed_data,Roboto_Thin_ttf_compressed_size,16.0f);
    AddFont(Roboto_ThinItalic_ttf_compressed_data,Roboto_ThinItalic_ttf_compressed_size,16.0f);
    AddFont(Roboto_Medium_ttf_compressed_data,Roboto_Medium_ttf_compressed_size, 24.0f);
    AddFont(Roboto_MediumItalic_ttf_compressed_data,Roboto_MediumItalic_ttf_compressed_size, 24.0f);
    AddFont(RobotoMono_Regular_ttf_compressed_data,RobotoMono_Regular_ttf_compressed_size, 16.0f);
    AddFont(RobotoMono_Medium_ttf_compressed_data,RobotoMono_Medium_ttf_compressed_size, 16.0f);
    AddFont(RobotoMono_Bold_ttf_compressed_data,RobotoMono_Bold_ttf_compressed_size, 16.0f);
}

int Utils::GetFontIndex(Fonts font)
{
    return static_cast<int>(font);
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

std::string Utils::decode64(const std::string &val)
{
    using namespace boost::archive::iterators;
    using It = transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>;
    return boost::algorithm::trim_right_copy_if(std::string(It(std::begin(val)), It(std::end(val))), [](char c) {
       return c == '\0';
    });
}
std::string Utils::encode64(const std::string &val)
{
    using namespace boost::archive::iterators;
    using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
    auto tmp = std::string(It(std::begin(val)), It(std::end(val)));
    return tmp.append((3 - val.size() % 3) % 3, '=');
}

stbi_uc * Utils::decodeImageData(stbi_uc const *buffer, int len, int *x, int *y, int *channels_in_file)
{
    return stbi_load_from_memory(buffer,len,x,y,channels_in_file,STBI_rgb_alpha);
}
stbi_uc * Utils::decodeGifData(stbi_uc const *buffer, int len, int *x, int *y,
                               int *channels_in_file, int *count, int** delays)
{
    return stbi_load_gif_from_memory(buffer, len, delays, x, y, count, channels_in_file, STBI_rgb_alpha);
}

gl_image_ptr Utils::loadImage(unsigned char* data, int width, int height, int channels)
{
    if(!data) return gl_image_ptr();
    auto image = std::make_shared<gl_image>();
    //https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    /*if(channels == 3)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height,
                     0, GL_RGB, GL_UNSIGNED_BYTE, data);
    }
    else if (channels == 4)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }*/
    image->width = width;
    image->height = height;
    image->channels = channels;
    image->textureId = image_texture;
    return image;
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
std::string Utils::getHumanReadableTimeAgo(uint64_t time)
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
    const auto s = std::chrono::duration_cast<std::chrono::seconds>(diff);
    if(s.count() > 0)
    {
        if(s.count() == 1) return "1 second ago";
        return fmt::format("{} seconds ago", s.count());
    }
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
    return fmt::format("{} miliseconds ago", ms.count());
}
