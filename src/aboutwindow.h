#ifndef ABOUTWINDOW_H
#define ABOUTWINDOW_H

#include "resizableglimage.h"
#include "markdown/markdownnodeblockparagraph.h"

class AboutWindow
{
public:
    AboutWindow();
    void setWindowShowing(bool flag);
    void showAboutWindow(int appFrameWidth,int appFrameHeight);
private:
    bool windowShowing = false;
    ResizableGLImagePtr applicationLogo;
    MarkdownNodeBlockParagraph githubParagraph;
    MarkdownNodeBlockParagraph licenseParagraph;
};

#endif // ABOUTWINDOW_H
