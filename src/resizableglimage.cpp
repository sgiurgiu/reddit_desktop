#include "resizableglimage.h"

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
