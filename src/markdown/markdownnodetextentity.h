#ifndef MARKDOWNNODETEXTENTITY_H
#define MARKDOWNNODETEXTENTITY_H

#include "markdownnode.h"

class MarkdownNodeTextEntity : public MarkdownNode
{
public:
    MarkdownNodeTextEntity(const char* text,size_t size);
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::TextEntity;
    }
private:
    std::string text;
};

#endif // MARKDOWNNODETEXTENTITY_H
