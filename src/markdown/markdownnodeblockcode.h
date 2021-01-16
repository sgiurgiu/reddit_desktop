#ifndef MARKDOWNNODEBLOCKCODE_H
#define MARKDOWNNODEBLOCKCODE_H

#include "markdownnode.h"
#include <md4c.h>
#include <string>

class MarkdownNodeBlockCode : public MarkdownNode
{
public:
    MarkdownNodeBlockCode(const MD_BLOCK_CODE_DETAIL*);
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::BlockCode;
    }
private:
    MD_BLOCK_CODE_DETAIL detail;
    std::string text;
    std::string lang;
    std::string info;
};

#endif // MARKDOWNNODEBLOCKCODE_H
