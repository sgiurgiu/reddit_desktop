#ifndef RESIZABLEINPUTTEXTMULTILINE_H
#define RESIZABLEINPUTTEXTMULTILINE_H

#include <imgui.h>

class ResizableInputTextMultiline
{
public:
    static bool InputText(const char* label, char* buf, size_t buf_size, ImVec2* size);
};

#endif // RESIZABLEINPUTTEXTMULTILINE_H
