#ifndef RESIZABLEINPUTTEXTMULTILINE_H
#define RESIZABLEINPUTTEXTMULTILINE_H

#include <imgui.h>
#include <string>

class ResizableInputTextMultiline
{
public:
    static bool InputText(const char* label, std::string* buf, ImVec2* size);
};

#endif // RESIZABLEINPUTTEXTMULTILINE_H
