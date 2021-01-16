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
#include <iostream>

#ifdef RD_WINDOWS
#include <shlobj.h>
#include <shlwapi.h>
#include <objbase.h>
#else
#include <boost/process/spawn.hpp>
#include <boost/process/search_path.hpp>
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

void Utils::AddFont(const unsigned int* fontData, const unsigned int fontDataSize, float fontSize)
{
    ImFontConfig fontAwesomeConfig;
    fontAwesomeConfig.MergeMode = true;
    fontAwesomeConfig.GlyphMinAdvanceX = fontSize; // Use if you want to make the icon monospaced
    static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

    ImFontConfig emojiConfig;
    emojiConfig.MergeMode = true;
    emojiConfig.GlyphMinAdvanceX = fontSize; // Use if you want to make the icon monospaced
    /*
     static const ImWchar emoji_icon_ranges[] = { 0x231A, 0x26CF,
                                                // 0x26D1, 0x1F251,
                                                 0 };
    */
    static const ImWchar romanian_ranges[] = { 0x0100, 0x017F,
                                               0x0180, 0x024F,
                                               0 };

    ImFontConfig config;
    config.MergeMode = true;

    ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(fontData,fontDataSize, fontSize);
    ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(fontData,fontDataSize, fontSize,&config,ImGui::GetIO().Fonts->GetGlyphRangesChineseFull());
    ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(fontData,fontDataSize, fontSize,&config,ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());
    ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(fontData,fontDataSize, fontSize,&config,ImGui::GetIO().Fonts->GetGlyphRangesKorean());
    ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(fontData,fontDataSize, fontSize,&config,ImGui::GetIO().Fonts->GetGlyphRangesJapanese());
    ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(fontData,fontDataSize, fontSize,&config,ImGui::GetIO().Fonts->GetGlyphRangesThai());
    ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(fontData,fontDataSize, fontSize,&config,ImGui::GetIO().Fonts->GetGlyphRangesVietnamese());
    ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(fontData,fontDataSize, fontSize,&config,romanian_ranges);

    ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(FontAwesome_compressed_data,
                                                         FontAwesome_compressed_size, fontSize,
                                                         &fontAwesomeConfig, icon_ranges);

    /*ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(NotoColorEmoji_ttf_compressed_data,
                                                         NotoColorEmoji_ttf_compressed_size, fontSize,
                                                         &emojiConfig,emoji_icon_ranges);*/
}

void Utils::LoadFonts()
{
    float normalFontSize = 20.f;
    AddFont(NotoSans_Black_ttf_compressed_data,NotoSans_Black_ttf_compressed_size,normalFontSize);
    AddFont(NotoSans_BlackItalic_ttf_compressed_data,NotoSans_BlackItalic_ttf_compressed_size,normalFontSize);
    AddFont(NotoSans_Bold_ttf_compressed_data,NotoSans_Bold_ttf_compressed_size,normalFontSize);
    AddFont(NotoSans_BoldItalic_ttf_compressed_data,NotoSans_BoldItalic_ttf_compressed_size,normalFontSize);
    AddFont(NotoSans_Italic_ttf_compressed_data,NotoSans_Italic_ttf_compressed_size,normalFontSize);
    AddFont(NotoSans_Light_ttf_compressed_data,NotoSans_Light_ttf_compressed_size,normalFontSize);
    AddFont(NotoSans_LightItalic_ttf_compressed_data,NotoSans_LightItalic_ttf_compressed_size,normalFontSize);
    AddFont(NotoSans_Medium_ttf_compressed_data,NotoSans_Medium_ttf_compressed_size, normalFontSize);
    AddFont(NotoSans_MediumItalic_ttf_compressed_data,NotoSans_MediumItalic_ttf_compressed_size, normalFontSize);
    AddFont(NotoSans_Regular_ttf_compressed_data,NotoSans_Regular_ttf_compressed_size,normalFontSize);
    AddFont(NotoSans_Thin_ttf_compressed_data,NotoSans_Thin_ttf_compressed_size,normalFontSize);
    AddFont(NotoSans_ThinItalic_ttf_compressed_data,NotoSans_ThinItalic_ttf_compressed_size,normalFontSize);
    AddFont(NotoSans_Medium_ttf_compressed_data,NotoSans_Medium_ttf_compressed_size, 24.0f);
    AddFont(NotoSans_MediumItalic_ttf_compressed_data,NotoSans_MediumItalic_ttf_compressed_size, 24.0f);
    AddFont(NotoMono_Regular_ttf_compressed_data,NotoMono_Regular_ttf_compressed_size, normalFontSize);
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
ResizableGLImagePtr Utils::loadBlurredImage(unsigned char* data, int width, int height, int channels)
{
    if(!data) return ResizableGLImagePtr();

    float sigma = 15;
    iir_gauss_blur(width, height, channels, data, sigma);
    GLuint FramebufferName = 0;
    glGenFramebuffers(1, &FramebufferName);
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

    auto image = loadImage(data,width,height,channels);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           image->textureId, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
    glViewport( 0, 0, width, height );
    glPushMatrix();  //Make sure our transformations don't affect any other transformations in other code
    glOrtho(0, width, 0, height, -1, 1);
    glTranslatef(width/6, height/3, 0.0f);  //Translate rectangle to its assigned x and y position
    glColor3f(1, 1, 1);
    glBegin(GL_LINE_LOOP);   //We want to draw a quad, i.e. shape with four sides
        glVertex2f(0,0);
        glVertex2f(0, height/3);
        glVertex2f(2*width/3, height/3);
        glVertex2f(2*width/3, 0);
    glEnd();
    glPopMatrix();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &FramebufferName);

    return image;
}
ResizableGLImagePtr Utils::loadImage(unsigned char* data, int width, int height, int channels)
{
    if(!data) return ResizableGLImagePtr();
    auto image = std::make_unique<ResizableGLImage>();

    //SDL_CreateTexture();
    //SDL_UpdateTexture()

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
    /*glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, data);*/
    if(channels == 3)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height,
                     0, GL_RGB, GL_UNSIGNED_BYTE, data);
    }
    else if (channels == 4)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }
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
ImVec4 Utils::GetDownVoteColor()
{
    return ImVec4(0.58f,0.58f,0.96f,1.0f);
}
ImVec4 Utils::GetUpVoteColor()
{
    return ImVec4(1.0f,0.54f,0.0f,1.0f);
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
