#ifndef MEDIAWIDGET_H
#define MEDIAWIDGET_H

#include <imgui.h>

class MediaWidget
{
public:
    static int mediaProgressBar(float mediaPosition, float mediaDuration, const ImVec2& size);
};

#endif // MEDIAWIDGET_H
