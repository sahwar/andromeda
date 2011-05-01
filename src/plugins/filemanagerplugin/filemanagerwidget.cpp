#include "filemanagerwidget.h"
#include "filemanagerwidget_p.h"

#include <QtGui/QKeyEvent>
#include <QDebug>

using namespace FileManagerPlugin;
using namespace CorePlugin;

void FileManagerWidgetPrivate::onDoubleClick(const QModelIndex &index)
{
    Q_Q(FileManagerWidget);

    QString path = model->filePath(index);
    QFileInfo info(path);
    if (info.isDir()) {
        q->setCurrentPath(info.canonicalFilePath());
    }
}

// Triggers when current item in history changes
void FileManagerWidgetPrivate::onCurrentItemIndexChanged(int index)
{
    Q_Q(FileManagerWidget);

    QString path = history->itemAt(index).path();
    if (currentPath != path) {
        currentPath = path;
        QModelIndex modelIndex = model->index(path);
        currentView->setRootIndex(modelIndex);

        emit q->currentPathChanged(path);
    }
}

FileManagerWidget::FileManagerWidget(QWidget *parent) :
    QWidget(parent),
    d_ptr(new FileManagerWidgetPrivate(this))
{
    Q_D(FileManagerWidget);

    QListView *listView = new QListView(this);
    QListView *iconView = new QListView(this);
    QColumnView *columnView = new QColumnView(this);
//    QTableView *tableView = new QTableView(this);
    QTreeView *tableView = new QTreeView(this);
    QTreeView *treeView = new QTreeView(this);

//    iconView->setGridSize(QSize(150, 100));
    iconView->setGridSize(QSize(100, 100));
    iconView->setWordWrap(true);
    iconView->setViewMode(QListView::IconMode);
    iconView->setFlow(QListView::LeftToRight);
    iconView->setMovement(QListView::Snap);
//    iconView->setMovement(QListView::Static);
    iconView->setResizeMode(QListView::Adjust);

    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setRootIsDecorated(false);
    tableView->setItemsExpandable(false);

    d->model = 0;
    d->currentView = 0;
    d->viewMode = (FileManagerWidget::ViewMode)-1; // to skip if in setView()
    d->layout = new QStackedLayout(this);

    d->views[ListView] = listView;
    d->views[IconView] = iconView;
    d->views[ColumnView] = columnView;
    d->views[TableView] = tableView;
    d->views[TreeView] = treeView;
    d->blockEvents = false;

    setFocusPolicy(Qt::StrongFocus);
    for (int i = 0; i < MaxViews; i++) {
        d->views[i]->setFocusProxy(this);
        d->views[i]->setSelectionMode(QAbstractItemView::ContiguousSelection);
        d->views[i]->setSelectionBehavior(QAbstractItemView::SelectRows);
        d->views[i]->setDragDropMode(QAbstractItemView::DragDrop);
        d->views[i]->setAcceptDrops(true);
        d->layout->addWidget(d->views[i]);
//        d->views[i]->setDragEnabled(true);
        connect(d->views[i], SIGNAL(doubleClicked(QModelIndex)),
                d, SLOT(onDoubleClick(QModelIndex)),
                Qt::QueuedConnection);
    }

    d->layout->setContentsMargins(0, 0, 0, 0);
    setLayout(d->layout);

    d->undoManager = new FileSystemUndoManager(this);
    connect(d->undoManager->undoStack(), SIGNAL(canUndoChanged(bool)), SIGNAL(canUndoChanged(bool)));

    d->history = new History(this);
    connect(d->history, SIGNAL(currentItemIndexChanged(int)), d, SLOT(onCurrentItemIndexChanged(int)));

    FileSystemModel *model = new FileSystemModel(this);
    model->setRootPath("/");
    model->setFilter(QDir::AllEntries | /*QDir::NoDotAndDotDot |*/ QDir::AllDirs | QDir::Hidden);
    model->setReadOnly(false);
    setModel(model);

    setViewMode(ListView);
}

FileManagerWidget::~FileManagerWidget()
{
    delete d_ptr;
}

FileManagerWidget::ViewMode FileManagerWidget::viewMode() const
{
    Q_D(const FileManagerWidget);

    return d->viewMode;
}

void FileManagerWidget::setViewMode(FileManagerWidget::ViewMode mode)
{
    Q_D(FileManagerWidget);

    if (d->viewMode != mode) {
        d->viewMode = mode;
        d->layout->setCurrentIndex(mode);
        d->currentView = d->views[mode];

        QModelIndex index = d->model->index(d->currentPath);
        d->currentView->setRootIndex(index);
    }
}

QFileSystemModel * FileManagerWidget::model() const
{
    Q_D(const FileManagerWidget);

    return d->model;
}

void FileManagerWidget::setModel(QFileSystemModel *model)
{
    Q_D(FileManagerWidget);

    d->model = model;

    for (int i = 0; i < MaxViews; i++) {
        d->views[i]->setModel(model);
    }
}

QString FileManagerWidget::currentPath() const
{
    Q_D(const FileManagerWidget);

    return d->currentPath;
}

History * FileManagerWidget::history() const
{
    Q_D(const FileManagerWidget);

    return d->history;
}

QStringList FileManagerWidget::selectedPaths() const
{
    Q_D(const FileManagerWidget);

    QStringList result;
    QModelIndexList list = d->currentView->selectionModel()->selectedRows();

    foreach (QModelIndex index, list) {
        result.append(d->model->filePath(index));
    }
    return result;
}

FileSystemUndoManager * FileManagerWidget::undoManager() const
{
    return d_func()->undoManager;
}

void FileManagerWidget::setCurrentPath(const QString &path)
{
    Q_D(FileManagerWidget);

    if (d->currentPath != path) {
        d->currentPath = path;
        QModelIndex index = d->model->index(path);
        d->currentView->setRootIndex(index);

        HistoryItem item(QIcon(), QDateTime::currentDateTime(), QFileInfo(path).fileName(), path);
        d->history->appendItem(item);

        emit currentPathChanged(path);
    }
}

void FileManagerWidget::copy()
{
    QClipboard * clipboard = QApplication::clipboard();
    QMimeData * data = new QMimeData();
    QList<QUrl> urls;

    QStringList paths = selectedPaths();
    qDebug() << paths;

    foreach (const QString &path, paths) {
        urls.append(QUrl::fromLocalFile(path));
    }
    data->setUrls(urls);
    clipboard->setMimeData(data);
}

void FileManagerWidget::paste()
{
    Q_D(FileManagerWidget);

    // TODO: use QtFileCopier
    QClipboard * clipboard = QApplication::clipboard();
    const QMimeData * data = clipboard->mimeData();
    QList<QUrl> urls = data->urls();
    QDir dir(currentPath());

    CopyCommand * command = new CopyCommand;
    command->setDestination(currentPath());

    foreach (QUrl path, urls) {
        QString filePath = path.toLocalFile();
        command->appendSourcePath(filePath);
    }
    d->undoManager->undoStack()->push(command);
}

bool removePath(const QString &path);

void FileManagerWidget::remove()
{
    QStringList paths = selectedPaths();
    foreach (const QString &path, paths) {
        bool result = removePath(path);
        if (!result) {
        }
    }
}

void FileManagerWidget::undo()
{
    undoManager()->undoStack()->undo();
}

void FileManagerWidget::redo()
{
    undoManager()->undoStack()->redo();
}

void FileManagerWidget::back()
{
    history()->back();
}

void FileManagerWidget::forward()
{
    history()->forward();
}

void FileManagerWidget::keyPressEvent(QKeyEvent *event)
{
    Q_D(FileManagerWidget);

    if (!d->blockEvents) { // prevent endless recursion
        d->blockEvents = true;
        qApp->sendEvent(d_func()->currentView, event);
        d->blockEvents = false;
    } else {
        d->blockEvents = false;
    }
}

void FileManagerWidget::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(FileManagerWidget);

    if (!d->blockEvents) { // prevent endless recursion
        d->blockEvents = true;
        qApp->sendEvent(d_func()->currentView, event);
        d->blockEvents = false;
    } else {
        d->blockEvents = false;
    }
}
