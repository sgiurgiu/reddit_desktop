#include "markdownnodeimage.h"

MarkdownNodeImage::MarkdownNodeImage(const MD_SPAN_IMG_DETAIL* detail):
    src(detail->src.text,detail->src.size),
    title(detail->title.text,detail->title.size)
{

}
void MarkdownNodeImage::Render()
{

}
