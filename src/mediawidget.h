#ifndef MEDIAWIDGET_H
#define MEDIAWIDGET_H

#include <imgui.h>
#include <cstdint>

class MediaWidget
{
public:
    static int mediaProgressBar(float mediaPosition,
                                float mediaDuration,
                                float cachedTime,
                                const ImVec2& size);
};

#endif // MEDIAWIDGET_H
