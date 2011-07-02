#include "mainwindow.h"
#include "mainwindow_p.h"

#include <QtCore/QFileInfo>
#include <QtGui/QAction>
#include <QtGui/QDesktopServices>
#include <QDebug>
#include <CorePlugin>

#include <actionmanager.h>
#include <commandcontainer.h>
#include <iview.h>
#include <perspectivewidget.h>

#include "mainwindowplugin.h"

using namespace MainWindowPlugin;
using namespace CorePlugin;
using namespace GuiSystem;

int MainWindowPrivate::currentIndex() const
{
    return tabWidget->currentIndex();
}

Page * MainWindowPrivate::currentPage() const
{
    return pages.at(currentIndex());
}

void MainWindowPrivate::onTextEntered(const QString &path)
{
    currentPage()->setCurrentPath(path);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    d_ptr(new MainWindowPrivate(this))
{
    Q_D(MainWindow);

    d->tabWidget = new QTabWidget;
    d->tabWidget->setDocumentMode(true);
    setCentralWidget(d->tabWidget);
    d->lineEdit = new EnteredLineEdit(this);
    d->toolBar = new QToolBar(this);

    QAction *backAction = new QAction(QIcon(":/images/icons/back.png"), "Back", this);
    QAction *forwardAction = new QAction(QIcon(":/images/icons/forward.png"), "Forward", this);

    d->toolBar->addAction(backAction);
    d->toolBar->addAction(forwardAction);
    d->toolBar->addSeparator();
    d->toolBar->addWidget(d->lineEdit);

    addToolBar(d->toolBar);
    setUnifiedTitleAndToolBarOnMac(true);

    ActionManager *manager = ActionManager::instance();
    setMenuBar(manager->container(MENU_BAR)->menuBar());

    connect(d->lineEdit, SIGNAL(textEntered(QString)), d, SLOT(onTextEntered(QString)));
    connect(backAction, SIGNAL(triggered()), SLOT(back()));
    connect(forwardAction, SIGNAL(triggered()), SLOT(forward()));

    resize(640, 480);
    newTab();
    newTab();
    newTab();

    QAction *a = new QAction(this);
//    a->setObjectName(ACTION_NEW_WINDOW);
    addAction(a);
}

MainWindow::~MainWindow()
{
    delete d_ptr;
}

void MainWindow::back()
{
    d_func()->currentPage()->history()->back();
}

void MainWindow::forward()
{
    d_func()->currentPage()->history()->forward();
}

void MainWindow::newTab()
{
    Q_D(MainWindow);

    PerspectiveWidget *widget = new PerspectiveWidget;
    Page *page = new Page;
    page->setPerspectiveWidget(widget);
    connect(page, SIGNAL(currentPathChanged(QString)), d->lineEdit, SLOT(setText(QString)));
    d->pages.append(page);
    d->tabWidget->addTab(widget, "tab");
    page->setCurrentPath(QDesktopServices::storageLocation(QDesktopServices::HomeLocation));
}

