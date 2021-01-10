#include "markdownnodeblockcode.h"

MarkdownNodeBlockCode::MarkdownNodeBlockCode(const MD_BLOCK_CODE_DETAIL* detail):
    detail(*detail)
{
    //detail is "shallow" copied, due to the MD_ATTRIBUTE struct
    //which contains char* and other arrays
    //Don't touch info and lang members, unless you deep copy them
    //they definitely don't contain any data that we care about at the moment,
    //as is not like we're gonna implement a language highlighter
}
