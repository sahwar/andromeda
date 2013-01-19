#include "filemanagerwidget.h"
#include "filemanagerwidget_p.h"

#include <QtCore/QDataStream>
#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtCore/QMetaProperty>

#include <QtGui/QAction>
#include <QtGui/QDesktopServices>
#include <QtGui/QHeaderView>
#include <QtGui/QFileDialog>
#include <QtGui/QKeyEvent>
#include <QtGui/QLineEdit>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QTextFrame>

#include <GuiSystem/ActionManager>
#include <GuiSystem/constants.h>
#include <IO/QDriveInfo>
#include <IO/QMimeDatabase>

#include "filemanagerconstants.h"
#include "filemanagerhistory_p.h"
#include "filemanagerhistoryitem_p.h"
#include "filemanagersettings.h"
#include "fileinfodialog.h"
#include "filemanagersettings.h"
#include "filemanagersettings_p.h"
#include "openwithmenu.h"

using namespace GuiSystem;
using namespace FileManager;

static QString getSuffix(const QString &fileName)
{
    QMimeDatabase db;

    QString suffix = db.suffixForFileName(fileName);
    if (suffix.isEmpty())
        suffix = QFileInfo(fileName).suffix();

    return suffix;
}

/*!
    \class FileTextEdit
    \internal
*/
FileTextEdit::FileTextEdit(QWidget *parent) :
    QTextEdit(parent)
{
}

void FileTextEdit::resizeEvent(QResizeEvent *e)
{
    QTextEdit::resizeEvent(e);
    if (alignment().testFlag(Qt::AlignVCenter))
        realignVCenter(this);
}

void FileTextEdit::showEvent(QShowEvent *e)
{
    QTextEdit::showEvent(e);
    if (alignment().testFlag(Qt::AlignVCenter))
        realignVCenter(this);
}

void FileTextEdit::realignVCenter(QTextEdit * pTextEdit)
{
    // retrieve total text document height and widget height
    int nDocHeight = (int)pTextEdit->document()->size().height();
    if(nDocHeight == 0)									// if no document, do nothing
        return;
    int nCtrlHeight= pTextEdit->height();				// the TextEdit control height

    // access the document top frame and its format
    QTextFrame * pFrame = pTextEdit->document()->rootFrame();
    QTextFrameFormat frameFmt = pFrame->frameFormat();

    // get current top margin and compute the 'black' height of the document
    qreal nTopMargin = (int)frameFmt.topMargin();			// the frame top margin
    qreal nBBDocH = nDocHeight - nTopMargin;				// the height of the document 'bounding box'

    // compute and set appropriate frame top margin
    if(nCtrlHeight <= nBBDocH)			// if the control is shorter than the document
        nTopMargin = 2;									// set a nominal top margin
    else								// if the control is taller than the document
        nTopMargin = (nCtrlHeight - nBBDocH)/2 + 2;		// set half of the excess as top margin
    frameFmt.setTopMargin(nTopMargin);

    // SET A BORDER!! (using the same colour as the background)
    frameFmt.setBorder(0.1);
    frameFmt.setBorderBrush(QColor(0xFFFFFF));
    pFrame->setFrameFormat(frameFmt);					// apply the new format
}

/*!
    \class FileDelegate
    \internal
*/
FileItemDelegate::FileItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

QWidget * FileItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &/*index*/) const
{
    QTextEdit *textEdit = new FileTextEdit(parent);
    if (textEdit) {
        textEdit->setAttribute(Qt::WA_MacShowFocusRect);
        textEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        textEdit->setAcceptRichText(false);
        textEdit->setAlignment(option.displayAlignment);
        const QStyleOptionViewItemV2 *optionV2 = qstyleoption_cast<const QStyleOptionViewItemV2 *>(&option);
        if (optionV2) {
            bool wrap = optionV2->features.testFlag(QStyleOptionViewItemV2::WrapText);
            textEdit->setWordWrapMode(wrap ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
        }
    }

    return textEdit;
}

void FileItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QTextEdit *textEdit = qobject_cast<QTextEdit*>(editor);
    if (textEdit) {
        QString text = index.data(Qt::EditRole).toString();
        textEdit->insertPlainText(text);

        QString suffix = getSuffix(text);

        if (!suffix.isEmpty()) {
            // The filename contains an extension. Assure that only the filename
            // gets selected.
            const int selectionLength = text.length() - suffix.length() - 1;
            QTextCursor cursor = textEdit->textCursor();
            cursor.movePosition(QTextCursor::StartOfBlock);
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, selectionLength);
            textEdit->setTextCursor(cursor);
        } else {
            textEdit->selectAll();
        }
    }
}

void FileItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QTextEdit *textEdit = qobject_cast<QTextEdit *>(editor);

    if (!textEdit)
        return;

    QString text = textEdit->toPlainText();

    QString oldSuffix = getSuffix(index.data(Qt::EditRole).toString());
    QString newSuffix = getSuffix(text);
    QString newName;
    if (newSuffix.isEmpty())
        newName = text;
    else
        newName = text.left(text.length() - newSuffix.length() - 1);

    if (FileManagerSettings::globalSettings()->warnOnExtensionChange() && oldSuffix != newSuffix) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Rename"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(tr("Are you sure you want to change the extension from \".%1\" to \".%2\"?").
                       arg(oldSuffix).arg(newSuffix));
        msgBox.setInformativeText("If you make this change, your file may open in a different application.");

        msgBox.addButton(tr("Use \".%1\"").arg(newSuffix), QMessageBox::AcceptRole);
        msgBox.addButton(tr("Keep \".%1\"").arg(oldSuffix), QMessageBox::RejectRole);
//        msgBox.setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowMinimizeButtonHint);

        if (msgBox.exec() == QMessageBox::RejectRole) {
            model->setData(index, QString(QLatin1String("%1.%2")).arg(newName).arg(oldSuffix), Qt::EditRole);
            return;
        }
    }

    model->setData(index, text, Qt::EditRole);
}

void FileItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::updateEditorGeometry(editor, option, index);
    QTextEdit *edit = qobject_cast<QTextEdit*>(editor);
    if (edit) {
        QRect r = editor->geometry();
        const int frameWidth = edit->frameWidth();

        const QStyleOptionViewItemV3 *optionV3 = qstyleoption_cast<const QStyleOptionViewItemV3 *>(&option);
        const QWidget *widget = optionV3 ? optionV3->widget : 0;
        const QStyle *style = widget ? widget->style() : qApp->style();
        const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, &option, widget) + 1;

        r.adjust(-frameWidth - textMargin, -frameWidth, frameWidth + textMargin, frameWidth);
        editor->setGeometry(r);
    }
}

bool FileItemDelegate::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return) {
            QTextEdit *editor = qobject_cast<QTextEdit *>(object);
            if (editor) {
                emit commitData(editor);
                emit closeEditor(editor, QAbstractItemDelegate::NoHint);
                return true;
            }
        }
    }
    return QStyledItemDelegate::eventFilter(object, event);
}

void FileManagerWidgetPrivate::setupUi()
{
    Q_Q(FileManagerWidget);

    layout = new QStackedLayout(q);
    layout->setContentsMargins(0, 0, 0, 0);

    q->setLayout(layout);
    q->setFocusPolicy(Qt::StrongFocus);
    q->setMinimumSize(200, 200);

    createActions();
    retranslateUi();
}

void FileManagerWidgetPrivate::createActions()
{
    Q_Q(FileManagerWidget);

    actions[FileManagerWidget::Open] = new QAction(this);
    actions[FileManagerWidget::Open]->setEnabled(false);
    actions[FileManagerWidget::Open]->setObjectName(Constants::Actions::Open);
    connect(actions[FileManagerWidget::Open], SIGNAL(triggered()), q, SLOT(open()));

    actions[FileManagerWidget::OpenInTab] = new QAction(this);
    actions[FileManagerWidget::OpenInTab]->setEnabled(false);
    actions[FileManagerWidget::OpenInTab]->setObjectName(Constants::Actions::OpenInTab);
    connect(actions[FileManagerWidget::OpenInTab], SIGNAL(triggered()), this, SLOT(openNewTab()));

    actions[FileManagerWidget::OpenInWindow] = new QAction(this);
    actions[FileManagerWidget::OpenInWindow]->setEnabled(false);
    actions[FileManagerWidget::OpenInWindow]->setObjectName(Constants::Actions::OpenInWindow);
    connect(actions[FileManagerWidget::OpenInWindow], SIGNAL(triggered()), this, SLOT(openNewWindow()));

    actions[FileManagerWidget::NewFolder] = new QAction(this);
    actions[FileManagerWidget::NewFolder]->setObjectName(Constants::Actions::NewFolder);
    connect(actions[FileManagerWidget::NewFolder], SIGNAL(triggered()), q, SLOT(newFolder()));

    actions[FileManagerWidget::Rename] = new QAction(this);
    actions[FileManagerWidget::Rename]->setEnabled(false);
    actions[FileManagerWidget::Rename]->setObjectName(Constants::Actions::Rename);
    connect(actions[FileManagerWidget::Rename], SIGNAL(triggered()), q, SLOT(rename()));

    actions[FileManagerWidget::MoveToTrash] = new QAction(this);
    actions[FileManagerWidget::MoveToTrash]->setEnabled(false);
    actions[FileManagerWidget::MoveToTrash]->setObjectName(Constants::Actions::MoveToTrash);
    connect(actions[FileManagerWidget::MoveToTrash], SIGNAL(triggered()), q, SLOT(moveToTrash()));

    actions[FileManagerWidget::Remove] = new QAction(this);
    actions[FileManagerWidget::Remove]->setEnabled(false);
    actions[FileManagerWidget::Remove]->setObjectName(Constants::Actions::Remove);
    connect(actions[FileManagerWidget::Remove], SIGNAL(triggered()), q, SLOT(remove()));

    actions[FileManagerWidget::ShowFileInfo] = new QAction(this);
    actions[FileManagerWidget::ShowFileInfo]->setObjectName(Constants::Actions::ShowFileInfo);
    connect(actions[FileManagerWidget::ShowFileInfo], SIGNAL(triggered()), q, SLOT(showFileInfo()));

    actions[FileManagerWidget::Redo] = new QAction(this);
    actions[FileManagerWidget::Redo]->setEnabled(false);
    actions[FileManagerWidget::Redo]->setObjectName(Constants::Actions::Redo);
    connect(actions[FileManagerWidget::Redo], SIGNAL(triggered()), q, SLOT(redo()));
    connect(q, SIGNAL(canRedoChanged(bool)), actions[FileManagerWidget::Redo], SLOT(setEnabled(bool)));

    actions[FileManagerWidget::Undo] = new QAction(this);
    actions[FileManagerWidget::Undo]->setEnabled(false);
    actions[FileManagerWidget::Undo]->setObjectName(Constants::Actions::Undo);
    connect(actions[FileManagerWidget::Undo], SIGNAL(triggered()), q, SLOT(undo()));
    connect(q, SIGNAL(canUndoChanged(bool)), actions[FileManagerWidget::Undo], SLOT(setEnabled(bool)));

    actions[FileManagerWidget::Cut] = new QAction(this);
    actions[FileManagerWidget::Cut]->setEnabled(false);
    actions[FileManagerWidget::Cut]->setObjectName(Constants::Actions::Cut);
//    connect(actions[FileManagerWidget::Cut], SIGNAL(triggered()), q, SLOT(cut()));

    actions[FileManagerWidget::Copy] = new QAction(this);
    actions[FileManagerWidget::Copy]->setObjectName(Constants::Actions::Copy);
    connect(actions[FileManagerWidget::Copy], SIGNAL(triggered()), q, SLOT(copy()));

    actions[FileManagerWidget::Paste] = new QAction(this);
    actions[FileManagerWidget::Paste]->setObjectName(Constants::Actions::Paste);
    connect(actions[FileManagerWidget::Paste], SIGNAL(triggered()), q, SLOT(paste()));

    actions[FileManagerWidget::MoveHere] = new QAction(this);
    actions[FileManagerWidget::MoveHere]->setObjectName(Constants::Actions::MoveHere);
    connect(actions[FileManagerWidget::MoveHere], SIGNAL(triggered()), q, SLOT(moveHere()));

    actions[FileManagerWidget::SelectAll] = new QAction(this);
    actions[FileManagerWidget::SelectAll]->setObjectName(Constants::Actions::SelectAll);
    connect(actions[FileManagerWidget::SelectAll], SIGNAL(triggered()), q, SLOT(selectAll()));

    actions[FileManagerWidget::ShowHiddenFiles] = new QAction(this);
    actions[FileManagerWidget::ShowHiddenFiles]->setCheckable(true);
    actions[FileManagerWidget::ShowHiddenFiles]->setObjectName(Constants::Actions::ShowHiddenFiles);
    connect(actions[FileManagerWidget::ShowHiddenFiles], SIGNAL(triggered(bool)), q, SLOT(showHiddenFiles(bool)));

    viewModeGroup = new QActionGroup(this);

    actions[FileManagerWidget::IconMode] = new QAction(viewModeGroup);
    actions[FileManagerWidget::ColumnMode] = new QAction(viewModeGroup);
    actions[FileManagerWidget::TreeMode] = new QAction(viewModeGroup);

    actions[FileManagerWidget::IconMode]->setCheckable(true);
    actions[FileManagerWidget::IconMode]->setChecked(true);
    actions[FileManagerWidget::ColumnMode]->setCheckable(true);
    actions[FileManagerWidget::TreeMode]->setCheckable(true);

    actions[FileManagerWidget::IconMode]->setData(FileManagerWidget::IconView);
    actions[FileManagerWidget::ColumnMode]->setData(FileManagerWidget::ColumnView);
    actions[FileManagerWidget::TreeMode]->setData(FileManagerWidget::TreeView);

    actions[FileManagerWidget::IconMode]->setObjectName(Constants::Actions::IconMode);
    actions[FileManagerWidget::ColumnMode]->setObjectName(Constants::Actions::ColumnMode);
    actions[FileManagerWidget::TreeMode]->setObjectName(Constants::Actions::TreeMode);

    connect(actions[FileManagerWidget::IconMode], SIGNAL(triggered(bool)), SLOT(toggleViewMode(bool)));
    connect(actions[FileManagerWidget::ColumnMode], SIGNAL(triggered(bool)), SLOT(toggleViewMode(bool)));
    connect(actions[FileManagerWidget::TreeMode], SIGNAL(triggered(bool)), SLOT(toggleViewMode(bool)));

    sortByGroup = new QActionGroup(this);

    actions[FileManagerWidget::SortByName] = new QAction(sortByGroup);
    actions[FileManagerWidget::SortBySize] = new QAction(sortByGroup);
    actions[FileManagerWidget::SortByType] = new QAction(sortByGroup);
    actions[FileManagerWidget::SortByDate] = new QAction(sortByGroup);
    actions[FileManagerWidget::SortDescendingOrder] = new QAction(this);

    actions[FileManagerWidget::SortByName]->setCheckable(true);
    actions[FileManagerWidget::SortBySize]->setCheckable(true);
    actions[FileManagerWidget::SortByType]->setCheckable(true);
    actions[FileManagerWidget::SortByDate]->setCheckable(true);
    actions[FileManagerWidget::SortDescendingOrder]->setCheckable(true);

    actions[FileManagerWidget::SortByName]->setChecked(true);

    actions[FileManagerWidget::SortByName]->setData(FileManagerWidget::NameColumn);
    actions[FileManagerWidget::SortBySize]->setData(FileManagerWidget::SizeColumn);
    actions[FileManagerWidget::SortByType]->setData(FileManagerWidget::TypeColumn);
    actions[FileManagerWidget::SortByDate]->setData(FileManagerWidget::DateColumn);

    actions[FileManagerWidget::SortByName]->setObjectName(Constants::Actions::SortByName);
    actions[FileManagerWidget::SortBySize]->setObjectName(Constants::Actions::SortBySize);
    actions[FileManagerWidget::SortByType]->setObjectName(Constants::Actions::SortByType);
    actions[FileManagerWidget::SortByDate]->setObjectName(Constants::Actions::SortByDate);

    connect(actions[FileManagerWidget::SortByName], SIGNAL(triggered(bool)), SLOT(toggleSortColumn(bool)));
    connect(actions[FileManagerWidget::SortBySize], SIGNAL(triggered(bool)), SLOT(toggleSortColumn(bool)));
    connect(actions[FileManagerWidget::SortByType], SIGNAL(triggered(bool)), SLOT(toggleSortColumn(bool)));
    connect(actions[FileManagerWidget::SortByDate], SIGNAL(triggered(bool)), SLOT(toggleSortColumn(bool)));
    connect(actions[FileManagerWidget::SortDescendingOrder], SIGNAL(triggered(bool)), SLOT(toggleSortOrder(bool)));

    for (int i = 0; i < FileManagerWidget::ActionCount; i++) {
        q->addAction(actions[i]);
    }
}

void FileManagerWidgetPrivate::retranslateUi()
{
    actions[FileManagerWidget::Open]->setText(tr("Open"));
    actions[FileManagerWidget::OpenInTab]->setText(tr("Open in new tab"));
    actions[FileManagerWidget::OpenInWindow]->setText(tr("Open in new window"));

    actions[FileManagerWidget::NewFolder]->setText(tr("New Folder"));
    actions[FileManagerWidget::Rename]->setText(tr("Rename"));
    actions[FileManagerWidget::MoveToTrash]->setText(tr("Move to trash"));
    actions[FileManagerWidget::Remove]->setText(tr("Remove"));
    actions[FileManagerWidget::ShowFileInfo]->setText(tr("File info"));

    actions[FileManagerWidget::Redo]->setText(tr("Redo"));
    actions[FileManagerWidget::Undo]->setText(tr("Undo"));
    actions[FileManagerWidget::Cut]->setText(tr("Cut"));
    actions[FileManagerWidget::Copy]->setText(tr("Copy"));
    actions[FileManagerWidget::Paste]->setText(tr("Paste"));
    actions[FileManagerWidget::MoveHere]->setText(tr("Move object(s) here"));
    actions[FileManagerWidget::SelectAll]->setText(tr("Select all"));

    actions[FileManagerWidget::ShowHiddenFiles]->setText(tr("Show hidden files"));

    actions[FileManagerWidget::IconMode]->setText(tr("Icon view"));
    actions[FileManagerWidget::ColumnMode]->setText(tr("Column view"));
    actions[FileManagerWidget::TreeMode]->setText(tr("Tree view"));

    actions[FileManagerWidget::SortByName]->setText(tr("Sort by name"));
    actions[FileManagerWidget::SortBySize]->setText(tr("Sort by size"));
    actions[FileManagerWidget::SortByType]->setText(tr("Sort by type"));
    actions[FileManagerWidget::SortByDate]->setText(tr("Sort by date"));
    actions[FileManagerWidget::SortDescendingOrder]->setText(tr("Descending order"));
}

class ListView : public QListView
{
public:
    ListView(QWidget *parent = 0) : QListView(parent) {}

    void dropEvent(QDropEvent *e)
    {
        QListView::dropEvent(e);
        doItemsLayout(); // fix for wrong layout after dropping
    }
};

void FileManagerWidgetPrivate::setFileSystemManager(FileSystemManager *manager)
{
    Q_Q(FileManagerWidget);

    if (fileSystemManager) {
        disconnect(fileSystemManager, 0, q, 0);
    }

    fileSystemManager = manager;
    connect(fileSystemManager, SIGNAL(canUndoChanged(bool)),
            q, SIGNAL(canUndoChanged(bool)));
    connect(fileSystemManager, SIGNAL(canRedoChanged(bool)),
            q, SIGNAL(canRedoChanged(bool)));
}

void FileManagerWidgetPrivate::setModel(FileSystemModel *m)
{
    Q_Q(FileManagerWidget);

    if (m == 0)
        setModel(new FileSystemModel(this));

    if (model == m)
        return;

    if (model) {
        if (currentView)
            disconnect(currentView->selectionModel(), 0, q, 0);
    }

    if (model && model->QObject::parent() == this)
        delete model;

    model = m;

    if (currentView) {
        currentView->setModel(model);
        QTreeView *view = qobject_cast<QTreeView *>(currentView);
        if (view) {
            view->setColumnWidth(0, 250);
            view->sortByColumn(sortingColumn, sortingOrder);
        }
        connect(currentView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                q, SIGNAL(selectedPathsChanged()));
    }
    connect(q, SIGNAL(selectedPathsChanged()), SLOT(onSelectionChanged()));
}

QModelIndexList FileManagerWidgetPrivate::selectedIndexes() const
{
    if (viewMode == FileManagerWidget::ColumnView)
        return currentView->selectionModel()->selectedIndexes();

    return currentView->selectionModel()->selectedRows();
}

void FileManagerWidgetPrivate::updateSorting()
{
    QTreeView *view = qobject_cast<QTreeView*>(currentView);
    if (view)
        view->sortByColumn(sortingColumn, sortingOrder);

    model->sort(sortingColumn, sortingOrder);
}

void FileManagerWidgetPrivate::paste(bool copy)
{
    QClipboard * clipboard = QApplication::clipboard();
    const QMimeData * data = clipboard->mimeData();
    const QList<QUrl> & urls = data->urls();

    QStringList files;
    foreach (const QUrl &url, urls) {
        files.append(url.toLocalFile());
    }

    if (copy)
        fileSystemManager->copy(files, currentPath);
    else
        fileSystemManager->move(files, currentPath);
}

bool FileManagerWidgetPrivate::hasFiles(const QStringList &paths)
{
    // TODO: use model for speedup
    foreach (const QString &path, paths) {
        if (!QFileInfo(path).isDir()) {
            return true;
        }
    }
    return false;
}

QAbstractItemView * FileManagerWidgetPrivate::createView(FileManagerWidget::ViewMode mode)
{
    Q_Q(FileManagerWidget);

    QAbstractItemView *view = 0;
    switch (mode) {
    case FileManagerWidget::IconView:
        view = createListView();
        break;
    case FileManagerWidget::ColumnView:
        view = createColumnView();
        break;
    case FileManagerWidget::TreeView:
        view = createTreeView();
        break;
    default:
        break;
    }

    view->setFocusProxy(q);
    view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setDragDropMode(QAbstractItemView::DragDrop);
    view->setAcceptDrops(true);
    view->setDefaultDropAction(Qt::MoveAction);
    view->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
    view->setTextElideMode(Qt::ElideMiddle);
    view->setItemDelegate(new FileItemDelegate(view));
    view->setIconSize(iconSizes[mode]);
    connect(view, SIGNAL(activated(QModelIndex)),
            this, SLOT(onActivated(QModelIndex)),
            Qt::QueuedConnection);
    if (model) {
        view->setModel(model);
        QTreeView *treeView = qobject_cast<QTreeView *>(view);
        if (treeView) {
            treeView->setColumnWidth(0, 250);
            treeView->sortByColumn(sortingColumn, sortingOrder);
        }
        connect(view->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                q, SIGNAL(selectedPathsChanged()));
    }

    return view;
}

QListView * FileManagerWidgetPrivate::createListView()
{
    Q_Q(FileManagerWidget);

    QListView *iconView = new ListView(q);

    iconView->setWordWrap(true);
    iconView->setWrapping(true);
    iconView->setFlow(QListView::LeftToRight);
    iconView->setViewMode(QListView::IconMode);
    iconView->setResizeMode(QListView::Adjust);
    iconView->setMovement(QListView::Static);

    updateListViewFlow(iconView);

    return iconView;
}

QColumnView *FileManagerWidgetPrivate::createColumnView()
{
    Q_Q(FileManagerWidget);

    return new QColumnView(q);
}

QTreeView * FileManagerWidgetPrivate::createTreeView()
{
    Q_Q(FileManagerWidget);

    QTreeView *treeView = new QTreeView(q);

    treeView->setAlternatingRowColors(alternatingRowColors);
    treeView->setExpandsOnDoubleClick(false);
    treeView->setItemsExpandable(itemsExpandable);
    treeView->setRootIsDecorated(itemsExpandable);
    treeView->setSortingEnabled(true);

    connect(treeView->header(), SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)),
            this, SLOT(onSortIndicatorChanged(int,Qt::SortOrder)));

    return treeView;
}

QAbstractItemView *FileManagerWidgetPrivate::testCurrentView(FileManagerWidget::ViewMode mode)
{
    switch (mode) {
    case FileManagerWidget::IconView:
        return qobject_cast<QListView*>(currentView);
    case FileManagerWidget::ColumnView:
        return qobject_cast<QColumnView*>(currentView);
    case FileManagerWidget::TreeView:
        return qobject_cast<QTreeView*>(currentView);
    default:
        break;
    }
    return 0;
}

void FileManagerWidgetPrivate::updateListViewFlow(QListView *view)
{
    QSize s = gridSize;
    if (flow == FileManagerWidget::LeftToRight) {
        view->setFlow(QListView::LeftToRight);
        view->setViewMode(QListView::IconMode);
        view->update();
    } else if (flow == FileManagerWidget::TopToBottom) {
        view->setFlow(QListView::TopToBottom);
        view->setViewMode(QListView::ListMode);
        s.setWidth(256);
    }
    view->setGridSize(s);
    // fixes dragging
    view->setDragEnabled(true);
    view->viewport()->setAcceptDrops(true);
    view->setMouseTracking(true);
}

void FileManagerWidgetPrivate::onActivated(const QModelIndex &index)
{
    Q_Q(FileManagerWidget);

    QString path = model->filePath(index);

//    Qt::KeyboardModifiers modifiers = qApp->keyboardModifiers();
//#ifdef Q_OS_MAC
//    if (modifiers & Qt::AltModifier) {
//#else
//    if (modifiers & Qt::MetaModifier) {
//#endif
//        emit q->openNewTabRequested(QStringList() << path);
//        return;
//    } else if (modifiers & Qt::ControlModifier) {
//        emit q->openNewWindowRequested(QStringList() << path);
//        return;
//    }

    QFileInfo info(path);
    if (info.isDir() && !info.isBundle()) {
        q->setCurrentPath(info.absoluteFilePath());
    } else {
        emit q->openRequested(QStringList() << path);
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

void FileManagerWidgetPrivate::onSortIndicatorChanged(int logicalIndex, Qt::SortOrder order)
{
    Q_Q(FileManagerWidget);

    if (sortingColumn == logicalIndex && sortingOrder == order)
        return;

    sortingColumn = (FileManagerWidget::Column)logicalIndex;
    sortingOrder = order;

    emit q->sortingChanged();
}

void FileManagerWidgetPrivate::openNewTab()
{
    Q_Q(FileManagerWidget);

    QStringList paths = q->selectedPaths();

    if (!paths.isEmpty())
        emit q->openNewTabRequested(paths);
}

void FileManagerWidgetPrivate::openNewWindow()
{
    Q_Q(FileManagerWidget);

    QStringList paths = q->selectedPaths();

    if (!paths.isEmpty())
        emit q->openNewWindowRequested(paths);
}

void FileManagerWidgetPrivate::toggleViewMode(bool toggled)
{
    Q_Q(FileManagerWidget);

    if (toggled) {
        QAction *action = qobject_cast<QAction*>(sender());
        if (action)
            q->setViewMode((FileManagerWidget::ViewMode)action->data().toInt());
    }
}

void FileManagerWidgetPrivate::toggleSortColumn(bool toggled)
{
    Q_Q(FileManagerWidget);

    if (toggled) {
        QAction *action = qobject_cast<QAction*>(sender());
        if (action)
            q->setSortingColumn((FileManagerWidget::Column)action->data().toInt());
    }
}

void FileManagerWidgetPrivate::toggleSortOrder(bool descending)
{
    Q_Q(FileManagerWidget);

    q->setSortingOrder(descending ? Qt::DescendingOrder : Qt::AscendingOrder);
}

void FileManagerWidgetPrivate::onSelectionChanged()
{
    Q_Q(FileManagerWidget);

    QStringList paths = q->selectedPaths();
    bool enabled = !paths.empty();

    actions[FileManagerWidget::Open]->setEnabled(enabled);
    actions[FileManagerWidget::OpenInTab]->setEnabled(enabled);
    actions[FileManagerWidget::OpenInWindow]->setEnabled(enabled);
    actions[FileManagerWidget::Rename]->setEnabled(enabled);
    actions[FileManagerWidget::MoveToTrash]->setEnabled(enabled);
    actions[FileManagerWidget::Remove]->setEnabled(enabled);
//    actions[FileManagerWidget::Cut]->setEnabled(enabled);
    actions[FileManagerWidget::Copy]->setEnabled(enabled);

    if (!paths.isEmpty()) {
        if (paths.size() == 1) {
            actions[FileManagerWidget::Cut]->setText(tr("Cut \"%1\"").arg(QFileInfo(paths[0]).fileName()));
            actions[FileManagerWidget::Copy]->setText(tr("Copy \"%1\"").arg(QFileInfo(paths[0]).fileName()));
        } else {
            actions[FileManagerWidget::Cut]->setText(tr("Cut %1 items").arg(paths.size()));
            actions[FileManagerWidget::Copy]->setText(tr("Copy %1 items").arg(paths.size()));
        }
    } else {
        actions[FileManagerWidget::Cut]->setText(tr("Cut"));
        actions[FileManagerWidget::Copy]->setText(tr("Copy"));
    }
}

static QDir::Filters mBaseFilters = QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs;

/*!
    \class FileManager::FileManagerWidget
    \brief FileManagerWidget is a main widget class for displaying filesystem.

    It provides basic functionality of a file manager - possibility to change
    view mode, copy/paste and undo/redo support, possbility to change files and
    folders sort order, etc.

    Also it provides actions for basic file manipulation - creating new folders,
    removing files, showing file info.

    \image html filemanagerwidget.png
*/

/*!
    \enum FileManager::FileManagerWidget::ViewMode
    This enum describes possible view modes can be used in FileManagerWidget.

    \var FileManager::FileManagerWidget::ViewMode FileManager::FileManagerWidget::IconView
    Is an icon or a list view, depending on file manager settings.

    \var FileManager::FileManagerWidget::ViewMode FileManager::FileManagerWidget::ColumnView
    Is a column view - each folder represented using a column.

    \var FileManager::FileManagerWidget::ViewMode FileManager::FileManagerWidget::TreeView
    Is a tree or a detailed list, depending on file manager settings.
*/

/*!
    \enum FileManager::FileManagerWidget::Flow
    This enum describes possible flow values for a list view mode.

    \var FileManager::FileManagerWidget::ViewMode FileManager::FileManagerWidget::LeftToRight
    The items are laid out in the view from the left to the right.

    \var FileManager::FileManagerWidget::ViewMode FileManager::FileManagerWidget::TopToBottom
    The items are laid out in the view from the top to the bottom.
*/

/*!
    \enum FileManager::FileManagerWidget::Column
    This enum describes column currently being sorted.

    \var FileManager::FileManagerWidget::Column FileManager::FileManagerWidget::NameColumn
    Name column.

    \var FileManager::FileManagerWidget::Column FileManager::FileManagerWidget::SizeColumn
    Size column.

    \var FileManager::FileManagerWidget::Column FileManager::FileManagerWidget::TypeColumn
    Type column.

    \var FileManager::FileManagerWidget::Column FileManager::FileManagerWidget::DateColumn
    Date column.
*/

/*!
    \enum FileManager::FileManagerWidget::Action
    This enum describes the types of actions which can be triggered in a file
    manager widget.

    \var FileManager::FileManagerWidget::Action FileManager::FileManagerWidget::NoAction
    Refers to an invalid action.

    \var FileManager::FileManagerWidget::Action FileManager::FileManagerWidget::Open
    Open current file in an external editor or folder in current widget.

    \var FileManager::FileManagerWidget::Action FileManager::FileManagerWidget::OpenInTab
    Open current file or folder in new tab.

    \var FileManager::FileManagerWidget::Action FileManager::FileManagerWidget::OpenInWindow
    Open current file or folder in new window.

    \var FileManager::FileManagerWidget::Action FileManager::FileManagerWidget::NewFolder
    Create new folder in current dir.

    \var FileManager::FileManagerWidget::Action FileManager::FileManagerWidget::Rename
    Rename selected file or folder.

    \var FileManager::FileManagerWidget::Action FileManager::FileManagerWidget::MoveToTrash
    Move selected files and/or folders to the trash.

    \var FileManager::FileManagerWidget::Action FileManager::FileManagerWidget::Remove
    Permanently remove selected paths from filesystem.

    \var FileManager::FileManagerWidget::Action FileManager::FileManagerWidget::ShowFileInfo
    Show FileManager::FileInfoDialog for each selected path.

    \var FileManager::FileManagerWidget::Action FileManager::FileManagerWidget::Redo
    Redo next operation.

    \var FileManager::FileManagerWidget::Action FileManager::FileManagerWidget::Undo
    Undo previous operation.

    \var FileManager::FileManagerWidget::Action FileManager::FileManagerWidget::Copy
    Copy selected paths to a clipboard.

    \var FileManager::FileManagerWidget::Action FileManager::FileManagerWidget::Paste
    Copy files and folders located at paths in a clipboard to a current folder.

    \var FileManager::FileManagerWidget::Action FileManager::FileManagerWidget::MoveHere
    Move files and folders located at paths in a clipboard to a current folder.

    \var FileManager::FileManagerWidget::Action FileManager::FileManagerWidget::SelectAll
    Select all paths at a current location.

    \var FileManager::FileManagerWidget::Action FileManager::FileManagerWidget::ShowHiddenFiles
    Toggle showing of hidden files.

    \var FileManager::FileManagerWidget::Action FileManager::FileManagerWidget::IconMode
    Toggle icon view mode.

    \var FileManager::FileManagerWidget::Action FileManager::FileManagerWidget::ColumnMode
    Toggle column view mode.

    \var FileManager::FileManagerWidget::Action FileManager::FileManagerWidget::TreeMode
    Toggle tree view mode.

    \var FileManager::FileManagerWidget::Action FileManager::FileManagerWidget::SortByName
    Toggle sorting by name.

    \var FileManager::FileManagerWidget::Action FileManager::FileManagerWidget::SortBySize
    Toggle sorting by size.

    \var FileManager::FileManagerWidget::Action FileManager::FileManagerWidget::SortByType
    Toggle sorting by type.

    \var FileManager::FileManagerWidget::Action FileManager::FileManagerWidget::SortByDate
    Toggle sorting by date.

    \var FileManager::FileManagerWidget::Action FileManager::FileManagerWidget::SortDescendingOrder
    Toggle sort order.
*/

/*!
    Creates FileManagerWidget with the given \a parent.
*/
FileManagerWidget::FileManagerWidget(QWidget *parent) :
    QWidget(parent),
    d_ptr(new FileManagerWidgetPrivate(this))
{
    Q_D(FileManagerWidget);

    qRegisterMetaType<ViewMode>("ViewMode");

    d->setupUi();

    d->blockKeyEvent = false;
    d->model = 0;
    d->currentView = 0;
    d->viewMode = (FileManagerWidget::ViewMode)-1; // to skip if in setView()
    d->fileSystemManager = 0;
    d->sortingColumn = (FileManagerWidget::Column)-1;
    d->sortingOrder = (Qt::SortOrder)-1;
    d->itemsExpandable = true;
    d->alternatingRowColors = true;

    d->history = new FileManagerHistory(this);
    connect(d->history, SIGNAL(currentItemIndexChanged(int)), d, SLOT(onCurrentItemIndexChanged(int)));

    FileSystemModel *model = new FileSystemModel(this);
    model->setRootPath("/");
    model->setFilter(mBaseFilters);
    model->setReadOnly(false);
    d->setModel(model);
    d->setFileSystemManager(FileSystemManager::instance());

    FileManagerSettings *settings = FileManagerSettings::globalSettings();

    setViewMode(IconView);
    setFlow((Flow)settings->flow());
    setIconSize(IconView, settings->iconSize(FileManagerSettings::IconView));
    setIconSize(ColumnView, settings->iconSize(FileManagerSettings::ColumnView));
    setIconSize(TreeView, settings->iconSize(FileManagerSettings::TreeView));
    setGridSize(settings->gridSize());
    setItemsExpandable(settings->itemsExpandable());
    setSorting(NameColumn, Qt::AscendingOrder);

    FileManagerSettings::globalSettings()->d_func()->addWidget(this);
}

/*!
    Destroys FileManagerWidget.
*/
FileManagerWidget::~FileManagerWidget()
{
    FileManagerSettings::globalSettings()->d_func()->removeWidget(this);

    delete d_ptr;
}

/*!
    Returns action specified by \a action enum.
*/
QAction * FileManagerWidget::action(Action action) const
{
    Q_D(const FileManagerWidget);

    if (action <= NoAction || action >= ActionCount)
        return 0;

    return d->actions[action];
}

/*!
    \property FileManagerWidget::alternatingRowColors
    Holds if view should use alternatingRowColors.  It only affects tree view
    mode.

    Default value is true.
*/
bool FileManagerWidget::alternatingRowColors() const
{
    Q_D(const FileManagerWidget);

    return d->alternatingRowColors;
}

void FileManagerWidget::setAlternatingRowColors(bool enable)
{
    Q_D(FileManagerWidget);

    if (d->alternatingRowColors == enable)
        return;

    QTreeView *view = qobject_cast<QTreeView*>(d->currentView);
    if (view)
        view->setAlternatingRowColors(enable);
}

/*!
    \property FileManagerWidget::canRedo
    Holds wheter widget can redo next operation.
*/
bool FileManagerWidget::canRedo() const
{
    return fileSystemManager()->canRedo();
}

/*!
    \fn void FileManagerWidget::canRedoChanged(bool canRedo)
    This signal is emited when FileManagerWidget::canRedo property is changed.
*/

/*!
    \property FileManagerWidget::canUndo
    Holds wheter widget can redo previsous operation.
*/
bool FileManagerWidget::canUndo() const
{
    return fileSystemManager()->canUndo();
}

/*!
    \fn void FileManagerWidget::canUndoChanged(bool canUndo)
    This signal is emited when FileManagerWidget::canUndo property is changed.
*/

/*!
    \property FileManagerWidget::currentPath
    This property holds current folder displayed it the widget.
*/
QString FileManagerWidget::currentPath() const
{
    Q_D(const FileManagerWidget);

    return d->currentPath;
}

void FileManagerWidget::setCurrentPath(const QString &path)
{
    Q_D(FileManagerWidget);

    if (d->currentPath != path) {
        d->currentPath = path;
        QModelIndex index = d->model->index(path);
        if (d->model->isDir(index)) {
            d->currentView->selectionModel()->clear(); // to prevent bug with selecting dir we enter in
            d->currentView->setRootIndex(index);

            FileManagerHistoryItemData item;
            item.path = path;
            item.title = QFileInfo(path).fileName();
            item.lastVisited = QDateTime::currentDateTime();
            d->history->d_func()->appendItem(FileManagerHistoryItem(item));

            emit currentPathChanged(path);
        }
    }
}

/*!
    \fn void FileManagerWidget::currentPathChanged(const QString &path)
    This signal is emitted when currently displayed folder changes.
*/

/*!
    \property FileManagerWidget::flow
    This property holds which direction the items layout should flow. It only
    affects icon view mode.

    Default value is FileManagerWidget::LeftToRight.
*/
FileManagerWidget::Flow FileManagerWidget::flow() const
{
    Q_D(const FileManagerWidget);

    return d->flow;
}

void FileManagerWidget::setFlow(FileManagerWidget::Flow flow)
{
    Q_D(FileManagerWidget);

    if (d->flow == flow)
        return;

    d->flow = flow;

    QListView *view = qobject_cast<QListView*>(d->currentView);
    if (view)
        d->updateListViewFlow(view);
}

/*!
    \property FileManagerWidget::gridSize
    This property holds the size of the layout grid. It only affects icon view
    mode.
*/
QSize FileManagerWidget::gridSize() const
{
    Q_D(const FileManagerWidget);

    return d->gridSize;
}

void FileManagerWidget::setGridSize(QSize s)
{
    Q_D(FileManagerWidget);

    if (d->gridSize == s)
        return;

    d->gridSize = s;

    QListView *view = qobject_cast<QListView*>(d->currentView);
    if (view) {
        if (d->flow == TopToBottom)
            s.setWidth(256);
        view->setGridSize(s);
    }
}

/*!
    Returns icon fixe for the view mode \a mode. Each view has indepentent icon
    size; that size is usually controlled by FileManagerSettings, i.e. when you
    change icon size using settings, it is changed in all views simultaneously.
*/
QSize FileManagerWidget::iconSize(FileManagerWidget::ViewMode mode) const
{
    Q_D(const FileManagerWidget);

    if (mode < 0 || mode >= FileManagerWidget::MaxViews)
        return QSize();

    return d->iconSizes[mode];
}

/*!
    Sets icon fixe for the view mode \a mode.
*/
void FileManagerWidget::setIconSize(FileManagerWidget::ViewMode mode, QSize size)
{
    Q_D(FileManagerWidget);

    if (mode < 0 && mode >= FileManagerWidget::MaxViews)
        return;

    if (d->iconSizes[mode] == size)
        return;

    d->iconSizes[mode] = size;

    QAbstractItemView *view = d->testCurrentView(mode);
    if (view)
        view->setIconSize(size);
}

/*!
    \property FileManagerWidget::itemsExpandable
    This property holds whether the items are expandable by the user. It only
    affects tree view mode.

    Default value is true.
*/
bool FileManagerWidget::itemsExpandable() const
{
    Q_D(const FileManagerWidget);

    return d->itemsExpandable;
}

void FileManagerWidget::setItemsExpandable(bool expandable)
{
    Q_D(FileManagerWidget);

    if (d->itemsExpandable == expandable)
        return;

    d->itemsExpandable = expandable;

    QTreeView *view = qobject_cast<QTreeView *>(d->currentView);
    if (view) {
        if (!expandable) {
            view->collapseAll();
            view->setRootIsDecorated(false);
            view->setItemsExpandable(false);
        } else {
            view->setRootIsDecorated(true);
            view->setItemsExpandable(true);
        }
    }
}

/*!
    \property FileManagerWidget::selectedPaths
    This property holds list of path (files or folders) currently selected by a user.
*/
QStringList FileManagerWidget::selectedPaths() const
{
    Q_D(const FileManagerWidget);

    QStringList result;
    foreach (const QModelIndex &index, d->selectedIndexes()) {
        result.append(d->model->filePath(index));
    }
    return QStringList(result);
}

/*!
    \fn void FileManagerWidget::selectedPathsChanged()
    This signal is emitted when user changes the selection in a view.
*/

/*!
    \property FileManagerWidget::sortingColumn
    This property holds the column which are currently sorted.
    Default value is FileManagerWidget::NameColumn.
*/
FileManagerWidget::Column FileManagerWidget::sortingColumn() const
{
    return d_func()->sortingColumn;
}

void FileManagerWidget::setSortingColumn(FileManagerWidget::Column column)
{
    Q_D(FileManagerWidget);

    if (d->sortingColumn == column)
        return;

    d->sortingColumn = column;
    d->updateSorting();

    for (int i = NameColumn; i < ColumnCount; i++) {
        d->actions[SortByName + i]->setChecked(column == NameColumn + i);
    }

    emit sortingChanged();
}

/*!
    \fn void FileManagerWidget::sortingChanged()
    This signal is emitted when FileManagerWidget::sortingColumn or
    FileManagerWidget::sortingOrder properties are changed.
*/

/*!
    \property FileManagerWidget::sortingOrder
    This property holds the order in which FileManagerWidget::sortingColumn is
    sorted.

    Default value is Qt::AscendingOrder.
*/
Qt::SortOrder FileManagerWidget::sortingOrder() const
{
    return d_func()->sortingOrder;
}

void FileManagerWidget::setSortingOrder(Qt::SortOrder order)
{
    Q_D(FileManagerWidget);

    if (d->sortingOrder == order)
        return;

    d->sortingOrder = order;
    d->updateSorting();

    d->actions[SortDescendingOrder]->setChecked(order == Qt::DescendingOrder);

    emit sortingChanged();
}

/*!
    Sets both sort \a column and sort \a order.
*/
void FileManagerWidget::setSorting(FileManagerWidget::Column column, Qt::SortOrder order)
{
    Q_D(FileManagerWidget);

    if (d->sortingColumn == column && d->sortingOrder == order)
        return;

    d->sortingColumn = column;
    d->sortingOrder = order;
    d->updateSorting();
    emit sortingChanged();
}

/*!
    \property FileManagerWidget::viewMode
    This property holds current view mode.

    Default value is FileManagerWidget::IconView.

    \sa FileManagerWidget::ViewMode
*/
FileManagerWidget::ViewMode FileManagerWidget::viewMode() const
{
    Q_D(const FileManagerWidget);

    return d->viewMode;
}

void FileManagerWidget::setViewMode(ViewMode mode)
{
    Q_D(FileManagerWidget);

    if (d->viewMode != mode) {
        d->viewMode = mode;
        bool focus = d->currentView ? d->currentView->hasFocus() : false;
        d->layout->setCurrentIndex(mode);
        delete d->currentView;
        d->currentView = d->createView(mode);
        d->layout->addWidget(d->currentView);
        if (focus) {
            d->currentView->setFocus();
        }

        QModelIndex index = d->model->index(d->currentPath);
        d->currentView->setRootIndex(index);

        d->actions[IconMode]->setChecked(mode == IconView);
        d->actions[ColumnMode]->setChecked(mode == ColumnView);
        d->actions[TreeMode]->setChecked(mode == TreeView);

        emit viewModeChanged(d->viewMode);
    }
}

/*!
    \fn void FileManagerWidget::viewModeChanged(FileManagerWidget::ViewMode mode)
    This signal is emitted when FileManagerWidget::viewMode property is changed;
    \a mode is a new view mode.
*/

/*!
    Returns pointer to a FileManager::FileSystemManager assotiated with this
    widget.
*/
FileSystemManager * FileManagerWidget::fileSystemManager() const
{
    Q_D(const FileManagerWidget);

    if (!d->fileSystemManager) {
        const_cast<FileManagerWidgetPrivate*>(d)->setFileSystemManager(d->model->fileSystemManager());
    }

    return d->fileSystemManager;
}

/*!
    Returns pointer to the FileManager::FileManagerHistory. Can be usefule if
    you need some specific history data (like list of items to display).
*/
FileManagerHistory *FileManagerWidget::history() const
{
    Q_D(const FileManagerWidget);

    return d->history;
}

/*!
    Returns pointer to a FileManager::FileSystemModel assotiated with this
    widget.
*/
FileSystemModel * FileManagerWidget::model() const
{
    Q_D(const FileManagerWidget);

    return d->model;
}

/*!
    Restores widget's \a state. Returns true on success.
    \sa saveState()
*/
bool FileManagerWidget::restoreState(const QByteArray &state)
{
    if (state.isEmpty())
        return false;

    QByteArray tmpState = state;
    QDataStream s(&tmpState, QIODevice::ReadOnly);

    quint8 tmp;
    QSize size;
    s >> tmp;
    setFlow((Flow)tmp);
    s >> size;
    setGridSize(size);
    s >> size;
    setIconSize(IconView, size);
    s >> tmp;
    setViewMode((FileManagerWidget::ViewMode)tmp);
    s >> tmp;
    setSortingColumn((FileManagerWidget::Column)tmp);
    s >> tmp;
    setSortingOrder((Qt::SortOrder)tmp);
    return true;
}

/*!
    Stores widget's state to a byte array.
    \sa restoreState()
*/
QByteArray FileManagerWidget::saveState() const
{
    QByteArray state;
    QDataStream s(&state, QIODevice::WriteOnly);

    s << (quint8)flow();
    s << gridSize();
    s << iconSize(IconView);
    s << (quint8)viewMode();
    s << (quint8)sortingColumn();
    s << (quint8)sortingOrder();

    return state;
}

/*!
    Clears current path and history.
*/
void FileManagerWidget::clear()
{
    Q_D(FileManagerWidget);

    setCurrentPath(QString());
    d->history->d_func()->items.clear();
    d->history->d_func()->currentItemIndex = -1;
}

QMenu * FileManagerWidget::createStandardMenu(const QStringList &paths)
{
    Q_D(FileManagerWidget);

    QMenu *menu = new QMenu;
    if (paths.isEmpty()) {
        menu->addAction(d->actions[NewFolder]);
        menu->addSeparator();
        menu->addAction(d->actions[ShowFileInfo]);
        menu->addSeparator();
        menu->addAction(d->actions[Paste]);
        menu->addAction(d->actions[MoveHere]);
        menu->addAction(d->actions[SelectAll]);
        menu->addSeparator();
        QMenu * viewModeMenu = menu->addMenu(tr("View Mode"));
        viewModeMenu->addAction(d->actions[IconMode]);
        viewModeMenu->addAction(d->actions[ColumnMode]);
        viewModeMenu->addAction(d->actions[TreeMode]);
        QMenu * sortByMenu = menu->addMenu(tr("Sort by"));
        sortByMenu->addAction(d->actions[SortByName]);
        sortByMenu->addAction(d->actions[SortByType]);
        sortByMenu->addAction(d->actions[SortBySize]);
        sortByMenu->addAction(d->actions[SortByDate]);
        sortByMenu->addSeparator();
        sortByMenu->addAction(d->actions[SortDescendingOrder]);
    } else {
        menu->addAction(d->actions[Open]);
        menu->addAction(d->actions[OpenInTab]);
        menu->addAction(d->actions[OpenInWindow]);

#ifdef QT_DEBUG
        OpenWithMenu *openWithMenu = new OpenWithMenu(menu);
        openWithMenu->setPaths(paths);
        menu->addMenu(openWithMenu);
#endif

        menu->addSeparator();
        menu->addAction(d->actions[ShowFileInfo]);
        menu->addSeparator();
        menu->addAction(d->actions[Rename]);
        menu->addAction(d->actions[MoveToTrash]);
        menu->addAction(d->actions[Remove]);
        menu->addSeparator();
        menu->addAction(d->actions[Copy]);
    }
    return menu;
}

/*!
    Creates new folder at the current folder.
*/
void FileManagerWidget::newFolder()
{
    Q_D(FileManagerWidget);
    QString dir = currentPath();
    if (dir == "") {
//        emit error;
    } else {
        QString  folderName = tr("New Folder");
        QModelIndex index = d->model->mkdir(d->model->index(dir), folderName);

        if (index.isValid())
            d->currentView->edit(index);
    }
}

/*!
    Opens currently selected paths. If selected only one folder, opens it;
    otherwise emits openRequested() signal.
*/
void FileManagerWidget::open()
{
    QStringList paths = selectedPaths();

    if (paths.count() == 1) {
        const QString &path = paths.first();
        QFileInfo info(path);
        if (info.isDir() && !info.isBundle()) {
            setCurrentPath(path);
            return;
        }
    }
    emit openRequested(paths);

//    foreach (const QString path, selectedPaths()) {
//        // TODO: open all folders in windows/tabs
//        QFileInfo info(path);
//        if (info.isDir() && !info.isBundle()) {
//            setCurrentPath(path);
//            return;
//        } else {
//            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
//        }
//    }
}

/*!
    Shows FileManager::FileInfoDialog for each selected path.
*/
void FileManagerWidget::showFileInfo()
{
    QStringList paths = selectedPaths();
    if (paths.isEmpty())
        paths.append(currentPath());

    foreach (const QString &path, paths) {
        FileInfoDialog *dialog = new FileInfoDialog(this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setFileInfo(QFileInfo(path));
        dialog->show();
    }
}

/*!
    Permanently removes selected paths from filesystem. Also shows warning dialog.
*/
void FileManagerWidget::remove()
{
    if (FileManagerSettings::globalSettings()->warnOnFileRemove()) {
        QMessageBox messageBox;
        messageBox.setWindowTitle(tr("Remove files"));
        messageBox.setIcon(QMessageBox::Warning);
        messageBox.setText(tr("Are you sure you want to delete selected item(s)?"));
        messageBox.setInformativeText(tr("This action can't be undone."));
        messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

        if (messageBox.exec() == QMessageBox::No)
            return;
    }

    fileSystemManager()->remove(selectedPaths());
}

/*!
    Starts editing of the selected path. Editing of multiple paths are not
    supported.
*/
void FileManagerWidget::rename()
{
    Q_D(FileManagerWidget);

    QModelIndexList indexes = d->selectedIndexes();
    if (indexes.count() != 1) {
//        emit error();
    } else {
        d->currentView->edit(indexes.first());
    }
}

/*!
    Moves selected path to the trash.
*/
void FileManagerWidget::moveToTrash()
{
    fileSystemManager()->moveToTrash(selectedPaths());
}

/*!
    Undoes last operation. Remove operation can't be undone.
*/
void FileManagerWidget::undo()
{
    fileSystemManager()->undo();
}

/*!
    Redoes next operation.
*/
void FileManagerWidget::redo()
{
    fileSystemManager()->redo();
}

/*!
    Copies selected paths to a clipboard.
*/
void FileManagerWidget::copy()
{
    QClipboard * clipboard = QApplication::clipboard();
    QMimeData * data = new QMimeData();
    QList<QUrl> urls;

    QStringList paths = selectedPaths();

    foreach (const QString &path, paths) {
        urls.append(QUrl::fromLocalFile(path));
    }
    data->setUrls(urls);
    clipboard->setMimeData(data);
}

/*!
    Copies files and folders located at paths in a clipboard to a current folder.
*/
void FileManagerWidget::paste()
{
    Q_D(FileManagerWidget);

    d->paste(true);
}

/*!
    Moves files and folders located at paths in a clipboard to a current folder.
*/
void FileManagerWidget::moveHere()
{
    Q_D(FileManagerWidget);

    d->paste(false);
}

/*!
    Selects all paths at a current location.
*/
void FileManagerWidget::selectAll()
{
    Q_D(FileManagerWidget);

    d->currentView->selectAll();
}

/*!
    Goes one step back in the history.
*/
void FileManagerWidget::back()
{
    history()->back();
}

/*!
    Goes one step forward in the history.
*/
void FileManagerWidget::forward()
{
    history()->forward();
}

/*!
    Goes one level up from the current folder.
*/
void FileManagerWidget::up()
{
    Q_D(FileManagerWidget);

    QDir dir(d->currentPath);
    dir.cdUp();
    setCurrentPath(dir.path());
}

/*!
    Enables or disables showing of hidden files.
*/
void FileManagerWidget::showHiddenFiles(bool show)
{
    Q_D(FileManagerWidget);

    if (show)
        d->model->setFilter(mBaseFilters | QDir::Hidden);
    else
        d->model->setFilter(mBaseFilters);
}

/*!
    Shows FileManagerWidget's context menu.
*/
void FileManagerWidget::showContextMenu(QPoint pos)
{
    QStringList paths = selectedPaths();
    QMenu *menu = createStandardMenu(paths);
    menu->exec(mapToGlobal(pos));
    delete menu;
}

/*!
    \fn void FileManagerWidget::openNewTabRequested(const QStringList &paths)
    This signal is emitted when user triggers OpenInTab action; \a paths is the
    list of currently selected files and/or folders.
*/

/*!
    \fn void FileManagerWidget::openNewWindowRequested(const QStringList &paths)
    This signal is emitted when user triggers OpenInWindow action; \a paths is the
    list of currently selected files and/or folders.
*/

/*!
    \reimp
*/
void FileManagerWidget::contextMenuEvent(QContextMenuEvent *e)
{
    showContextMenu(e->pos());
}

/*!
    \reimp
*/
void FileManagerWidget::keyPressEvent(QKeyEvent *event)
{
    Q_D(FileManagerWidget);

    if (d->blockKeyEvent) // prevent endless recursion
        return;

    switch (event->key()) {
    case Qt::Key_Down:
        if (event->modifiers() & Qt::ControlModifier) {
            open();
            break;
        }
    default:
        d->blockKeyEvent = true;
        qApp->sendEvent(d_func()->currentView, event);
        d->blockKeyEvent = false;
        break;
    }
}

/*!
    \reimp
*/
void FileManagerWidget::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(FileManagerWidget);

    if (!d->blockKeyEvent) { // prevent endless recursion
        d->blockKeyEvent = true;
        qApp->sendEvent(d_func()->currentView, event);
        d->blockKeyEvent = false;
    } else {
        d->blockKeyEvent = false;
    }
}

