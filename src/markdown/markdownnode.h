#ifndef MARKDOWNNODE_H
#define MARKDOWNNODE_H

#include <memory>
#include <vector>

class MarkdownNode
{
public:
    MarkdownNode();
    virtual void Render() = 0;
    void AddChild(std::unique_ptr<MarkdownNode> child);
    MarkdownNode* GetParent() const;
    size_t GetChildrenCount() const
    {
        return children.size();
    }
    enum class NodeType
    {
        Document,
        BlockQuote,
        BlockUnorderedList,
        BlockOrderedList,
        BlockListItem,
        ThematicBreak,
        Head,
        BlockCode,
        BlockHtml,
        BlockParagraph,
        Table,
        TableHead,
        TableBody,
        TableRow,
        TableCellHead,
        TableCell,
        Emphasis,
        Strong,
        Underline,
        Link,
        Image,
        Code,
        Strike,
        Break,
        SoftBreak,
        TextHtml,
        TextEntity,
        Text
    };
    virtual NodeType GetNodeType() const = 0;
protected:
    MarkdownNode* parent = nullptr;
    std::vector<std::unique_ptr<MarkdownNode>> children;
};

#endif // MARKDOWNNODE_H
