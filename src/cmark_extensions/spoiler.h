#ifndef REDDIT_EXTENSION_SPOILER_H
#define REDDIT_EXTENSION_SPOILER_H

#include "cmark-gfm-core-extensions.h"

#ifdef __cplusplus
extern "C" {
#endif

extern cmark_node_type CMARK_NODE_SPOILER;
cmark_syntax_extension *create_spoiler_extension(void);

#ifdef __cplusplus
}
#endif

#endif // REDDIT_EXTENSION_SPOILER_H
