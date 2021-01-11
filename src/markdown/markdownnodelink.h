#ifndef MARKDOWNNODELINK_H
#define MARKDOWNNODELINK_H

#include "markdownnode.h"
#include <md4c.h>

class MarkdownNodeLink : public MarkdownNode
{
public:
    MarkdownNodeLink(const MD_SPAN_A_DETAIL*);
    void Render() override;
    NodeType GetNodeType() const override
    {
        return NodeType::Link;
    }
private:
    std::string href;
    std::string title;
};

#endif // MARKDOWNNODELINK_H
