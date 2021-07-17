#include "utils.h"
#include "imgui.h"
#include "images/sprite_reddit.h"
#include "fonts/fonts.h"

#include "misc/freetype/imgui_freetype.h"
#include <array>
#include <fmt/format.h>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>
#include <chrono>
#include <iostream>
#include <locale>
#include <codecvt>

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

namespace {
    std::map<std::string, std::tuple<int,int,int,int>> thumbnailsCoordinates = {
        {"image",   {0,310      ,140,105}},
        {"default", {0,310+(148),140,105}},
        {"nsfw",    {0,310+(290),140,105}},
        {"spoiler", {0,310+(440),140,105}},
        {"self",    {0,310+(585),140,105}},
        {"reddit",  {0,310+(445),140,105}}, //same as "spoiler"
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

ResizableGLImagePtr Utils::redditDefaultSprites;

ImFont* Utils::AddFont(const std::filesystem::path& fontsFolder, const std::string& font, float fontSize)
{
    ImFontConfig fontAwesomeConfig;
    fontAwesomeConfig.MergeMode = true;
    fontAwesomeConfig.GlyphMinAdvanceX = fontSize; // Use if you want to make the icon monospaced
    static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    static const ImWchar romanian_ranges[] = { 0x0100, 0x017F,
                                               0x0180, 0x024F,
                                               0 };

    ImFontConfig config;
    config.MergeMode = true;

    auto fontFile = fontsFolder / font;
    auto fontFilenameString = fontFile.string();
    const char* fontFilename = fontFilenameString.c_str();

    ImGui::GetIO().Fonts->AddFontFromFileTTF(fontFilename, fontSize);
    ImGui::GetIO().Fonts->AddFontFromFileTTF(fontFilename, fontSize,&config,ImGui::GetIO().Fonts->GetGlyphRangesChineseFull());
    ImGui::GetIO().Fonts->AddFontFromFileTTF(fontFilename, fontSize,&config,ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());
    ImGui::GetIO().Fonts->AddFontFromFileTTF(fontFilename, fontSize,&config,ImGui::GetIO().Fonts->GetGlyphRangesKorean());
    ImGui::GetIO().Fonts->AddFontFromFileTTF(fontFilename, fontSize,&config,ImGui::GetIO().Fonts->GetGlyphRangesJapanese());
    ImGui::GetIO().Fonts->AddFontFromFileTTF(fontFilename, fontSize,&config,ImGui::GetIO().Fonts->GetGlyphRangesThai());
    ImGui::GetIO().Fonts->AddFontFromFileTTF(fontFilename, fontSize,&config,ImGui::GetIO().Fonts->GetGlyphRangesVietnamese());
    ImGui::GetIO().Fonts->AddFontFromFileTTF(fontFilename, fontSize,&config,romanian_ranges);

    ImFontConfig emojiConfig;
    emojiConfig.MergeMode = true;
    emojiConfig.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor;
    emojiConfig.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_Bitmap;
    emojiConfig.OversampleH = emojiConfig.OversampleV = 1;
    //emojiConfig.GlyphMinAdvanceX = fontSize;
    static const ImWchar emoji_icon_ranges[] = { 0x1, 0x1FFFF,
                                                 0 };
    auto seguiemjFile = (fontsFolder / "seguiemj.ttf").string();
    auto notoColorEmojiFile = (fontsFolder / "NotoColorEmoji.ttf").string();
    auto fontAwesomeFile = (fontsFolder / FONT_ICON_FILE_NAME_FA).string();

    ImGui::GetIO().Fonts->AddFontFromFileTTF(seguiemjFile.c_str(), fontSize,
                                                         &emojiConfig,emoji_icon_ranges);
    ImGui::GetIO().Fonts->AddFontFromFileTTF(notoColorEmojiFile.c_str(), fontSize,
                                                         &emojiConfig,emoji_icon_ranges);
    return ImGui::GetIO().Fonts->AddFontFromFileTTF(fontAwesomeFile.c_str(), fontSize,
                                                         &fontAwesomeConfig, icon_ranges);
}

void Utils::LoadFonts(const std::filesystem::path& executablePath)
{
#ifdef RD_WINDOWS
    const float normalFontSize = 18.f;
    const float bigFontSize = 22.f;
#else
    const float normalFontSize = 20.f;
    const float bigFontSize = 24.f;
#endif // WIN

    auto fontsFolder = executablePath / "fonts";
    if(!std::filesystem::exists(fontsFolder)) throw std::runtime_error(
                fmt::format("Fonts folder does not exists ({})",fontsFolder.string()));

    AddFont(fontsFolder, "NotoSans-Black.ttf", normalFontSize);
    AddFont(fontsFolder, "NotoSans-BlackItalic.ttf", normalFontSize);
    AddFont(fontsFolder, "NotoSans-Bold.ttf", normalFontSize);
    AddFont(fontsFolder, "NotoSans-BoldItalic.ttf", normalFontSize);
    AddFont(fontsFolder, "NotoSans-Italic.ttf", normalFontSize);
    AddFont(fontsFolder, "NotoSans-Light.ttf", normalFontSize);
    AddFont(fontsFolder, "NotoSans-LightItalic.ttf", normalFontSize);
    AddFont(fontsFolder, "NotoSans-Medium.ttf", normalFontSize);

    AddFont(fontsFolder, "NotoSans-MediumItalic.ttf", normalFontSize);
    AddFont(fontsFolder, "NotoSans-Regular.ttf", normalFontSize);

    AddFont(fontsFolder, "NotoSans-Thin.ttf", normalFontSize);
    AddFont(fontsFolder, "NotoSans-ThinItalic.ttf", normalFontSize);
    AddFont(fontsFolder, "NotoSans-Medium.ttf", bigFontSize);
    AddFont(fontsFolder, "NotoSans-MediumItalic.ttf", bigFontSize);
    AddFont(fontsFolder, "NotoMono-Regular.ttf", normalFontSize);
}
void Utils::DeleteFonts()
{
//    delete[] NotoSans_Black_ttf_compressed_data;
//    delete[] NotoSans_BlackItalic_ttf_compressed_data;
//    delete[] NotoSans_BoldItalic_ttf_compressed_data;
//    delete[] NotoSans_Italic_ttf_compressed_data;
//    delete[] NotoSans_Light_ttf_compressed_data;
//    delete[] NotoSans_LightItalic_ttf_compressed_data;
//    delete[] NotoSans_Medium_ttf_compressed_data;
//    delete[] NotoSans_MediumItalic_ttf_compressed_data;
//    delete[] NotoSans_Regular_ttf_compressed_data;
//    delete[] NotoSans_Thin_ttf_compressed_data;
//    delete[] NotoSans_ThinItalic_ttf_compressed_data;
//    delete[] NotoMono_Regular_ttf_compressed_data;
//    //delete[] NotoColorEmoji_ttf_compressed_data;
//    delete[] FontAwesome_compressed_data;
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
void Utils::LoadRedditThumbnails()
{
    int width, height, channels;
    auto data = decodeImageData(sprite_reddit_png,sprite_reddit_png_len,&width,&height,&channels);
    redditDefaultSprites = Utils::loadImage(data.get(),width,height,STBI_rgb_alpha);
}
void Utils::ReleaseRedditThumbnails()
{
    redditDefaultSprites.reset();
}
ResizableGLImagePtr Utils::GetRedditSpriteSubimage(int x, int y, int width, int height)
{
    auto image = std::make_unique<ResizableGLImage>();
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    //glTexStorage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glCopyImageSubData(redditDefaultSprites->textureId, GL_TEXTURE_2D, 0, x, y, 0,
                       image_texture, GL_TEXTURE_2D, 0, 0, 0, 0,
                       width, height, 1);

    image->width = width;
    image->height = height;
    image->channels = STBI_rgb_alpha;
    image->textureId = image_texture;

    return image;
}
ResizableGLImagePtr Utils::GetRedditHeader()
{
    int x = 0;
    int y = 1323;
    int width = 120;
    int height = 40;
    return GetRedditSpriteSubimage(x,y,width,height);
}
ResizableGLImagePtr Utils::GetRedditThumbnail(const std::string& kind)
{
    auto thumbnailIt = thumbnailsCoordinates.find(kind);
    if(thumbnailIt == thumbnailsCoordinates.end())
    {
        return ResizableGLImagePtr();
    }

    int x = std::get<0>(thumbnailIt->second);
    int y = std::get<1>(thumbnailIt->second);
    int width = std::get<2>(thumbnailIt->second);
    int height = std::get<3>(thumbnailIt->second);

    return GetRedditSpriteSubimage(x,y,width,height);
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
ResizableGLImagePtr Utils::loadBlurredImage(unsigned char* data, int width, int height, int channels)
{
    if(!data) return ResizableGLImagePtr();

    float sigma = 15;
    iir_gauss_blur(width, height, channels, data, sigma);
    /*GLuint FramebufferName = 0;
    glGenFramebuffers(1, &FramebufferName);
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);*/

    auto image = loadImage(data,width,height,channels);

    /*glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
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
    glDeleteFramebuffers(1, &FramebufferName);*/

    return image;
}
ResizableGLImagePtr Utils::loadImage(unsigned char* data, int width, int height, int channels)
{
    if(!data) return ResizableGLImagePtr();
    auto image = std::make_unique<ResizableGLImage>();

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
