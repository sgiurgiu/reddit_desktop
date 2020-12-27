#ifndef RESIZABLEGLIMAGE_H
#define RESIZABLEGLIMAGE_H

#include <memory>
#include <glad/glad.h>

class ResizableGLImage
{
public:
    ResizableGLImage();
    ~ResizableGLImage();
    GLuint textureId = 0;
    int width = 0;
    int height = 0;
    int channels = 0;
    int resizedWidth = 0;
    int resizedHeight = 0;
    bool isResized = false;
    float pictureRatio = 0.0f;
};

using ResizableGLImagePtr = std::shared_ptr<ResizableGLImage>;

#endif // RESIZABLEGLIMAGE_H
