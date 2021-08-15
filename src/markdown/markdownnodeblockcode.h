#ifndef MARKDOWNNODEBLOCKCODE_H
#define MARKDOWNNODEBLOCKCODE_H

#include "markdownnode.h"
#include <string>

class MarkdownNodeBlockCode : public MarkdownNode
{
public:
    MarkdownNodeBlockCode(std::string text, std::string lang, std::string info);
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::BlockCode;
    }
private:
    std::string text;
    std::string lang;
    std::string info;
};

#endif // MARKDOWNNODEBLOCKCODE_H
