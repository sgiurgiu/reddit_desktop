#include "utils.h"
#include "imgui.h"
#include "fonts/fonts.h"
#include <array>
#include <fmt/format.h>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>

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
