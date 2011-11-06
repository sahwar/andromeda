#ifndef HELLOWORLPLUGIN_H
#define HELLOWORLPLUGIN_H

#include <iplugin.h>
#include <commandcontainer.h>

class QAction;
class QModelIndex;
class QUrl;

namespace Bookmarks {
class BookmarksModel;
}

class BookmarksPluginImpl : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    explicit BookmarksPluginImpl();

    bool initialize(const QVariantMap &options);
    void shutdown();

private slots:
    void open(const QUrl &url);
    void openInTabs(const QList<QUrl> &urls);
    void openInWindow(const QList<QUrl> &urls);

    void showBookmarks();
    void addBookmark();
    void addFolder();

private:
    void createActions();
    void showBookmarkDialog(const QModelIndex &index, bool isFolder);
    void addDefaultBookmarks();

private:
    Bookmarks::BookmarksModel *model;

    QAction *addBookmarkAction;
    QAction *addFolderAction;
    QAction *showBookmarksAction;
};

class BookmarksToolBarContainer : public GuiSystem::CommandContainer
{
    Q_OBJECT

public:
    explicit BookmarksToolBarContainer(const QByteArray &id, QObject *parent = 0) :
        CommandContainer(id, parent)
    {}

    QToolBar *createToolBar() const;

signals:
    void open(const QUrl &url);
    void openInTabs(const QList<QUrl> &urls);

    void showBookmarksTriggered();
    void addBookmarkTriggered();
    void addFolderTriggered();
};

#endif // HELLOWORLPLUGIN_H