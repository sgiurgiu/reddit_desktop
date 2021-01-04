#include "resizableglimage.h"
#include <utility>

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
