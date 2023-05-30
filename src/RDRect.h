#ifndef RDRECT_H
#define RDRECT_H

#include <optional>
#include <algorithm>

struct RDRect
{
    RDRect(){}
    RDRect(int x, int y, int width, int height)
        : x{x}, y{y}, width{width}, height{height}
    {}

    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

std::optional<RDRect> getIntersection(RDRect const& r1, RDRect const& r2);

#endif // RDRECT_H
