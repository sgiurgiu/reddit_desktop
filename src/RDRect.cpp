#include "RDRect.h"

std::optional<RDRect> getIntersection(RDRect const& r1, RDRect const& r2)
{
    int x5 = std::max(r1.x, r2.x);
    int y5 = std::max(r1.y, r2.y);

    int x6 = std::min(r1.x + r1.width, r2.x + r2.width);
    int y6 = std::min(r1.y + r1.height, r2.y + r2.height);

    if (x5 > x6 || y5 > y6) {
       // No intersection;
        return {};
    }

    return RDRect{x5, y6, x6 - x5, y5 - y6};
}
