#include "utils.h"
#include "imgui.h"
#include "fonts/fonts.h"
#include <array>
#include <fmt/format.h>

void Utils::AddFont(const unsigned int* fontData, const unsigned int fontDataSize, float fontSize)
{
    ImFontConfig config;
    config.MergeMode = true;
    config.GlyphMinAdvanceX = 16.0f; // Use if you want to make the icon monospaced
    static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

    ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(fontData,fontDataSize, fontSize);
    ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(FontAwesome_compressed_data, FontAwesome_compressed_size, 16.0f,
                                                               &config, icon_ranges);

}

void Utils::LoadFonts()
{
    /*AddFont(Roboto_Black_ttf_compressed_data_base85,16.0f);
    AddFont(Roboto_BlackItalic_ttf_compressed_data_base85,16.0f);
    AddFont(Roboto_Bold_ttf_compressed_data_base85,16.0f);
    AddFont(Roboto_BoldItalic_ttf_compressed_data_base85,16.0f);
    AddFont(Roboto_Italic_ttf_compressed_data_base85,16.0f);
    AddFont(Roboto_Light_ttf_compressed_data_base85,16.0f);
    AddFont(Roboto_LightItalic_ttf_compressed_data_base85,16.0f);*/
    AddFont(Roboto_Medium_ttf_compressed_data,Roboto_Medium_ttf_compressed_size, 16.0f);
    AddFont(Roboto_MediumItalic_ttf_compressed_data,Roboto_MediumItalic_ttf_compressed_size, 16.0f);
    /*AddFont(Roboto_Regular_ttf_compressed_data_base85,16.0f);
    AddFont(Roboto_Thin_ttf_compressed_data_base85,16.0f);
    AddFont(Roboto_ThinItalic_ttf_compressed_data_base85,16.0f);*/
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
