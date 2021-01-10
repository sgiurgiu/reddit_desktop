#include "markdownnodelink.h"

MarkdownNodeLink::MarkdownNodeLink(const MD_SPAN_A_DETAIL* detail):
    href(detail->href.text,detail->href.size),
    title(detail->title.text,detail->title.size)
{
}
