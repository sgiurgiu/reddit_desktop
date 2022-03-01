#include "resizableglimage.h"
#include <utility>
#include <imgui.h>
#include <cmath>

ResizableGLImage::ResizableGLImage()
{        
}

ResizableGLImage::~ResizableGLImage()
{
    if(textureId > 0)
    {
        glDeleteTextures(1,&textureId);
    }
}
ResizableGLImage::ResizableGLImage ( ResizableGLImage &&other) noexcept :
    textureId(std::exchange(other.textureId,0)),
    width(std::exchange(other.width,0)),
    height(std::exchange(other.height,0)),
    channels(std::exchange(other.channels,0)),
    resizedWidth(std::exchange(other.resizedWidth,0)),
    resizedHeight(std::exchange(other.resizedHeight,0)),
    isResized(std::exchange(other.isResized,false)),
    pictureRatio(std::exchange(other.pictureRatio,0.f))
{
}
ResizableGLImage & ResizableGLImage::operator=( ResizableGLImage &&other) noexcept
{
    textureId     = std::exchange(other.textureId,0);
    width         = std::exchange(other.width,0);
    height        = std::exchange(other.height,0);
    channels      = std::exchange(other.channels,0);
    resizedWidth  = std::exchange(other.resizedWidth,0);
    resizedHeight = std::exchange(other.resizedHeight,0);
    isResized     = std::exchange(other.isResized,false);
    pictureRatio  = std::exchange(other.pictureRatio,0.f);
    return *this;
}

void ImGuiResizableGLImage(ResizableGLImage* img, float maxPictureHeight)
{
    if(!img)
        return;

    if(!img->isResized)
    {
        float width = (float)img->width;
        float height = (float)img->height;
        auto availableWidth = ImGui::GetContentRegionAvail().x * 0.9f;
        if(availableWidth > 100 && width > availableWidth)
        {
            //scale the picture
            float scale = availableWidth / width;
            width = availableWidth;
            height = scale * height;
        }
        //float maxPictureHeight = displayMode.h * 0.5f;
        if(maxPictureHeight > 100 && height > maxPictureHeight)
        {
            float scale = maxPictureHeight / height;
            height = maxPictureHeight;
            width = scale * width;
        }
        width = std::max(100.f,width);
        height = std::max(100.f,height);
        img->resizedWidth = width;
        img->resizedHeight = height;
        img->pictureRatio = width / height;
        img->isResized = true;
    }
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::ImageButton((void*)(intptr_t)img->textureId,ImVec2(img->resizedWidth,img->resizedHeight));
    ImGui::PopStyleColor(3);
    if(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow) &&
            ImGui::IsMouseHoveringRect(ImGui::GetItemRectMin(),ImGui::GetItemRectMax()) &&
            ImGui::IsMouseDragging(ImGuiMouseButton_Left)
            )
    {
        //keep same aspect ratio
        auto x = ImGui::GetIO().MouseDelta.x;
        auto y = ImGui::GetIO().MouseDelta.y;
        auto new_width = img->resizedWidth + x;
        auto new_height = img->resizedHeight + y;
        auto new_area = new_width * new_height;
        new_width = std::sqrt(img->pictureRatio * new_area);
        new_height = new_area / new_width;
        img->resizedWidth = std::max(100.f,new_width);
        img->resizedHeight = std::max(100.f,new_height);
    }

}
