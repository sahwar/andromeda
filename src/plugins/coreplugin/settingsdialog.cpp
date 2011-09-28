#include "settingsdialog.h"

#include "isettingspage.h"
#include "settingspagemanager.h"

#include <QtGui/QApplication>
#include <QtGui/QLineEdit>
#include <QtGui/QStandardItemModel>
#include <QtGui/QScrollBar>
#include <QtGui/QStyledItemDelegate>
#include <QtGui/QTabWidget>

// ----------- Category list view

class CategoryListViewDelegate : public QStyledItemDelegate
{
public:
    CategoryListViewDelegate(QObject *parent) : QStyledItemDelegate(parent) {}
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(qMax(size.height(), 32));
        return size;
    }
};

/**
 * Special version of a QListView that has the width of the first column as
 * minimum size.
 */
class CategoryListView : public QListView
{
public:
    CategoryListView(QWidget *parent = 0) : QListView(parent)
    {
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
        setItemDelegate(new CategoryListViewDelegate(this));
    }

    virtual QSize sizeHint() const
    {
        int width = sizeHintForColumn(0) + frameWidth() * 2 + 5;
        if (verticalScrollBar()->isVisible())
            width += verticalScrollBar()->width();
        return QSize(width, 100);
    }
};

namespace CorePlugin {

class SettingsDialogPrivate
{
    Q_DECLARE_PUBLIC(SettingsDialog)

public:
    SettingsDialogPrivate(SettingsDialog *qq) : q_ptr(qq) {}

    QLabel *headerLabel;
    QHBoxLayout *headerLayout;
    QStackedLayout *stackedLayout;
    QGridLayout *mainLayout;
    QListView *categoryList;

    QList<QString> categories;
    QList<QTabWidget *> tabWidgets;
    QMap<ISettingsPage *, QWidget *> widgets;

    SettingsPageManager *manager;
    QStandardItemModel *model;

    void addCategory(const QString &id);
    void addPage(ISettingsPage *page);
    void removePage(ISettingsPage *page);

    void setupUi();

private:
    SettingsDialog *q_ptr;
};

} // namespace CorePlugin

using namespace CorePlugin;

const int categoryIconSize = 24;

void SettingsDialogPrivate::addCategory(const QString &id)
{
    if (categories.contains(id))
        return;

    ISettingsPage *page = manager->pages(id).first();

    QStandardItem *item = new QStandardItem;
    item->setIcon(page->categoryIcon());
    item->setText(page->categoryName());
    item->setData(categories.size());

    model->appendRow(item);
    categories.append(id);

    QTabWidget *widget = new QTabWidget;
    stackedLayout->addWidget(widget);
    tabWidgets.append(widget);
}

void SettingsDialogPrivate::addPage(ISettingsPage *page)
{
    int index = categories.indexOf(page->category());

    QTabWidget *tabWidget = static_cast<QTabWidget *>(stackedLayout->widget(index));
    QWidget *widget = page->createPage(tabWidget);
    widgets.insert(page, widget);
    tabWidget->addTab(widget, page->name());
}

void SettingsDialogPrivate::removePage(ISettingsPage *page)
{
    QWidget *widget = widgets.take(page);
    if (widget) {
        delete widget;
    }
}

void SettingsDialogPrivate::setupUi()
{
    Q_Q(SettingsDialog);

    headerLabel = new QLabel;
    headerLabel->setAlignment(Qt::AlignHCenter);

    // Header label with large font and a bit of spacing (align with group boxes)
    QFont headerLabelFont = headerLabel->font();
    headerLabelFont.setBold(true);
    // Paranoia: Should a font be set in pixels...
    const int pointSize = headerLabelFont.pointSize();
    if (pointSize > 0)
        headerLabelFont.setPointSize(pointSize + 2);
    headerLabel->setFont(headerLabelFont);
    headerLabel->setAlignment(Qt::AlignLeft);

//    const int leftMargin = qApp->style()->pixelMetric(QStyle::PM_LayoutLeftMargin);
//    QSpacerItem *spacer = new QSpacerItem(leftMargin, 0, QSizePolicy::Fixed, QSizePolicy::Ignored);

    headerLayout = new QHBoxLayout;
//    headerLayout->addSpacerItem(spacer);
    headerLayout->addWidget(headerLabel);
    headerLayout->setSpacing(0);
    headerLayout->setMargin(0);

    categoryList = new CategoryListView;
    categoryList->setIconSize(QSize(categoryIconSize, categoryIconSize));
    categoryList->setSelectionMode(QAbstractItemView::SingleSelection);
    categoryList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    categoryList->setEditTriggers(QAbstractItemView::NoEditTriggers);

    stackedLayout = new QStackedLayout;
    stackedLayout->setMargin(0);

    mainLayout = new QGridLayout;
    mainLayout->addLayout(headerLayout,  0, 1, 1, 1);
    mainLayout->addWidget(categoryList,  0, 0, 2, 1);
    mainLayout->addLayout(stackedLayout, 1, 1, 1, 1);
    mainLayout->setColumnStretch(1, 4);
    q->setLayout(mainLayout);
    q->setMinimumSize(1024, 576);
}

/*!
    \class SettingsDialog

    \brief SettingsDialog represents application preferences.
*/

/*!
    \brief Creates an empty SettingsDialog with the given \a parent.
*/
SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    d_ptr(new SettingsDialogPrivate(this))
{
    Q_D(SettingsDialog);

    d->manager = 0;
    d->model = new QStandardItemModel(this);
    d->setupUi();

    d->categoryList->setModel(d->model);
    connect(d->categoryList->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(currentChanged(QModelIndex)), Qt::UniqueConnection);

    setObjectName(QLatin1String("SettingsDialog"));
}

/*!
    \brief Destroys SettingsDialog.
*/
SettingsDialog::~SettingsDialog()
{
    delete d_ptr;
}

/*!

    \brief Returns SettingsManager assigned to this dialog.
*/
SettingsPageManager *SettingsDialog::settingsPageManager() const
{
    return d_func()->manager;
}

/*!
    \brief Assigned \a manager to this dialog fill it with settings pages.
*/
void SettingsDialog::setSettingsPageManager(SettingsPageManager *manager)
{
    Q_D(SettingsDialog);

    if (d->manager) {
        // clear;
        disconnect(d->manager, 0, this, 0);
    }

    d->manager = manager;

    foreach (const QString &category, manager->categories()) {
        d->addCategory(category);
        foreach (ISettingsPage *page, d->manager->pages(category)) {
            d->addPage(page);
        }
    }
    connect(d->manager, SIGNAL(pageAdded(ISettingsPage*)), SLOT(onPageAdded(ISettingsPage*)));
    connect(d->manager, SIGNAL(pageRemoved(ISettingsPage*)), SLOT(onPageRemoved(ISettingsPage*)));
}

/*!
    \internal
*/
void SettingsDialog::currentChanged(const QModelIndex &current)
{
    Q_D(SettingsDialog);

    if (current.isValid()) {
        d->stackedLayout->setCurrentIndex(current.data(Qt::UserRole + 1).toInt());
        d->headerLabel->setText(current.data(Qt::DisplayRole).toString());
    }
}

/*!
    \internal
*/
void SettingsDialog::onPageAdded(ISettingsPage *page)
{
    Q_D(SettingsDialog);

    QString category = page->category();
    d->addCategory(category);
    d->addPage(page);
}

/*!
    \internal
*/
void SettingsDialog::onPageRemoved(ISettingsPage *page)
{
    d_func()->removePage(page);
}
