#ifndef MARKDOWNNODE_H
#define MARKDOWNNODE_H

#include <memory>
#include <vector>
#include <boost/signals2.hpp>

enum class NodeAlign
{
    AlignDefault,
    AlignLeft,
    AlignCenter,
    AlignRight
};

class MarkdownNode
{
public:
    MarkdownNode();
    virtual ~MarkdownNode() = default;
    virtual void Render() = 0;
    virtual void AddChild(std::unique_ptr<MarkdownNode> child);
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
    using IndividualComponentSignal = boost::signals2::signal<void(void)>;
    template<typename S>
    void addIndividualComponentSignal(S slot)
    {
        individualComponentSignal.connect(slot);
    }
protected:
    MarkdownNode* parent = nullptr;
    std::vector<std::unique_ptr<MarkdownNode>> children;
    IndividualComponentSignal individualComponentSignal;
};

#endif // MARKDOWNNODE_H
