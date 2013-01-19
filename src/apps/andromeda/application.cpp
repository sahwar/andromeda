#include "application.h"

#include <QtCore/QStringList>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QLocale>
#include <QtCore/QSettings>
#include <QtCore/QTranslator>

#include <QtGui/QIcon>
#include <QtGui/QDesktopServices>
#include <QtGui/QMenuBar>
#include <QtGui/QMessageBox>
#include <QtGui/QSystemTrayIcon>

#include <ExtensionSystem/ErrorsDialog>
#include <ExtensionSystem/PluginManager>
#include <ExtensionSystem/PluginView>

#include <GuiSystem/Action>
#include <GuiSystem/ActionManager>
#include <GuiSystem/Command>
#include <GuiSystem/CommandContainer>
#include <GuiSystem/EditorWindowFactory>
#include <GuiSystem/MenuBarContainer>
#include <GuiSystem/SettingsPageManager>
#include <GuiSystem/SettingsWindow>
#include <GuiSystem/constants.h>

#include <Widgets/WindowsMenu>

#include "browserwindow.h"
#include "browserwindow_p.h"
#include "commandssettingspage.h"
#include "generalsettingspage.h"
#include "settingsmodel.h"
#include "settingswidget.h"

using namespace ExtensionSystem;
using namespace GuiSystem;
using namespace Andromeda;

// returns root path of the application
static QString getRootPath()
{
    // Figure out root:  Up one from 'bin' or 'MacOs'
    QDir rootDir = QApplication::applicationDirPath();
    rootDir.cdUp();
    return rootDir.canonicalPath();
}

static inline QString getPluginPath()
{
    const QString rootDirPath = getRootPath();
    // Build path
    QString pluginPath = rootDirPath;
#if defined Q_OS_MACX
    pluginPath += QLatin1Char('/');
    pluginPath += QLatin1String("PlugIns");
#elif defined Q_OS_WIN
    pluginPath += QLatin1Char('/');
    pluginPath += QLatin1String("plugins");
#elif defined Q_OS_UNIX
    // not Mac UNIXes
    pluginPath += QLatin1Char('/');
    pluginPath += QLatin1String("lib");
    pluginPath += QLatin1String(LIB_SUFFIX);
    pluginPath += QLatin1Char('/');
    pluginPath += QLatin1String("andromeda");
    pluginPath += QLatin1Char('/');
    pluginPath += QLatin1String("plugins");
#endif
    return pluginPath;
}

static const qint32 andromedaAppMagic = 0x6330386e; // "c08n"
static const qint8 andromedaAppVersion = 1;

static QUrl urlFromUserInput(const QString &currentPath, const QString &text)
{
    // we try to resolve local paths
    QFileInfo info(text);
    if (info.isAbsolute()) {
        if (info.exists())
            return QUrl::fromLocalFile(info.canonicalFilePath());
        else
            return QUrl::fromUserInput(text);
    } else {
        QDir dir(currentPath);
        QString path = QDir::cleanPath(dir.absoluteFilePath(text));
        QFileInfo info(path);
        if (info.exists())
            return QUrl::fromLocalFile(path);
        else
            return QUrl::fromUserInput(text);
    }
}

class DockContainer : public CommandContainer
{
public:
    explicit DockContainer(const QByteArray &id, QObject *parent = 0);
    ~DockContainer();

    QMenu *createMenu(QWidget *parent) const;

private:
    QPointer<WindowsMenu> m_menu;
};

class WindowsContainer : public CommandContainer
{
public:
    explicit WindowsContainer(const QByteArray &id, QObject *parent = 0);
    ~WindowsContainer();

    QMenu *createMenu(QWidget *parent) const;

private:
    QPointer<WindowsMenu> m_menu;
};

DockContainer::DockContainer(const QByteArray &id, QObject *parent) :
    CommandContainer(id, parent),
    m_menu(new WindowsMenu)
{
}

DockContainer::~DockContainer()
{
    delete m_menu;
}

QMenu * DockContainer::createMenu(QWidget * /*parent*/) const
{
    return m_menu;
}

WindowsContainer::WindowsContainer(const QByteArray &id, QObject *parent) :
    CommandContainer(id, parent),
    m_menu(new WindowsMenu)
{
}

WindowsContainer::~WindowsContainer()
{
    delete m_menu;
}

QMenu * WindowsContainer::createMenu(QWidget *parent) const
{
    return m_menu;
}

Application::Application(int &argc, char **argv) :
    QtSingleApplication("Andromeda", argc, argv),
    m_pluginManager(0),
    m_firstStart(true)
{
    setOrganizationName("arch");
    setApplicationName("andromeda");

#ifdef ICON_LOCATION
    setWindowIcon(QIcon(ICON_LOCATION + QString("/andromeda.png")));
#endif

    setQuitOnLastWindowClosed(false);
    addLibraryPath(getPluginPath());

    m_pluginManager = new PluginManager(this);
    m_pluginManager->setPluginsFolder("andromeda");
    m_pluginManager->setTranslations(QStringList() <<
                                     "andromeda" <<
                                     "extensionsystem" <<
                                     "filemanager" <<
                                     "guisystem" <<
                                     "imageviewer" <<
                                     "widgets");
    connect(m_pluginManager, SIGNAL(pluginsLoaded()), SLOT(restoreSession()));

    EditorWindowFactory::setDefaultfactory(new BrowserWindowFactory(this));

    m_settingsPageManager = new SettingsPageManager(this);
    m_settingsPageManager->setObjectName("settingsPageManager");
    m_settingsPageManager->addPage(new GeneralSettingsPage(this));
    m_settingsPageManager->addPage(new CommandsSettingsPage(this));
    m_pluginManager->addObject(m_settingsPageManager);

    createActions();

    connect(this, SIGNAL(messageReceived(QString)), SLOT(handleMessage(QString)));
#ifndef Q_OS_MAC
    connect(this, SIGNAL(lastWindowClosed()), SLOT(exit()));
#endif

#ifdef Q_OS_MAC
    setTrayIconVisible(false);
#endif
}

Application::~Application()
{
    delete dockMenu;
    delete trayIcon;
#ifdef Q_OS_MAC
    delete menuBar;
#endif
}

Application *Application::instance()
{
    return qobject_cast<Application *>(QApplication::instance());
}

bool Application::activateApplication()
{
    QStringList arguments = this->arguments();
    Q_ASSERT(!arguments.isEmpty());
    arguments[0] = QDir::currentPath();
    return sendMessage(arguments.join("\n"));
}

bool Application::loadPlugins()
{
    m_pluginManager->loadPlugins();

    MenuBarContainer::instance()->retranslateUi();
    retranslateGoToMenu();

    QStringList args = arguments().mid(1);
    args.prepend(QDir::currentPath());
    handleArguments(args);
    m_pluginManager->postInitialize(arguments().mid(1));

    if (m_pluginManager->hasErrors()) {
        ErrorsDialog dlg;
        dlg.setMessage(tr("Errors occured during loading plugins."));
        dlg.setErrors(m_pluginManager->errors());
        dlg.exec();
    }

    if (m_pluginManager->plugins().isEmpty())
        return false;

    return true;
}

bool Application::isTrayIconVisible() const
{
    return trayIcon->isVisible();
}

void Application::setTrayIconVisible(bool visible)
{
    trayIcon->setVisible(visible);
}

/*!
    Creates new BrowserWindow.
*/
void Application::newWindow()
{
    BrowserWindow::newWindow();
}

/*!
    Shows plugins list.
*/
void Application::showPluginView()
{
    PluginView view;
    view.exec();
}

/*!
    Shows all settings.
*/
void Application::showSettings()
{
    SettingsWidget *widget = new SettingsWidget;
    widget->setAttribute(Qt::WA_DeleteOnClose);

    SettingsModel *settingsModel = new SettingsModel(widget);
    QSettings *settings = new QSettings(settingsModel);
    settingsModel->setSettings(settings);
    widget->setModel(settingsModel);

    widget->show();
}

/*!
    Shows preferences window.
*/
void Application::preferences()
{
    // TODO: test unloading pages while dialog is running
    if (!settingsWindow) {
        settingsWindow = new SettingsWindow();
        settingsWindow->setAttribute(Qt::WA_DeleteOnClose);
        settingsWindow->setSettingsPageManager(m_settingsPageManager);
        settingsWindow->restoreState(settingsWindowState);
        settingsWindow->installEventFilter(this);
        settingsWindow->show();
    } else {
        settingsWindow->raise();
        settingsWindow->activateWindow();
    }
}

void Application::restoreSession()
{
#ifdef Q_OS_MAC
    // Create menu bar now
    menuBar = MenuBarContainer::instance()->menuBar();
#endif

    loadSettings();

    bool ok = true;
    QString dataPath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    QString filePath = dataPath + QLatin1Char('/') + QLatin1String("session");
    QFile f(filePath);
    if (ok)
        ok = f.open(QFile::ReadOnly);

    if (ok) {
        QByteArray state = f.readAll();
        ok = restoreApplicationState(state);
        if (!ok)
            qWarning() << tr("Couldn't restore session (located at %1)").arg(filePath);
    }

    // We couldn't load session, fallback to creating window and opening default path
    if (!ok)
        newWindow();
}

void Application::saveSession()
{
    QString dataPath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    QFile f(dataPath + QLatin1Char('/') + QLatin1String("session"));

    saveSettings();

    if (!f.open(QFile::WriteOnly))
        return;

    f.write(saveApplicationState());
}

/*!
    Saves session and exits application.
*/
void Application::exit()
{
    saveSession();

    qDeleteAll(BrowserWindow::windows());

    QMetaObject::invokeMethod(PluginManager::instance(), "unloadPlugins", Qt::QueuedConnection);
    QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
}

/*!
    Shows "about" dialog.
*/
void Application::about()
{
    QString text = tr("<h3>Andromeda %1</h3><br/>"
                      "Revision %2<br/>"
                      "<br/>"
                      "Copyright 2010-2011 %3<br/>"
                      "Bugreports send to %4<br/>"
                      "<br/>"
                      "This is alpha version.<br/>"
                      "<br/>"
                      "The program is provided as is with no warranty of any kind, "
                      "including the warranty of design, merchantability "
                      "and fitness for a particular purpose.").
            arg(QLatin1String(PROJECT_VERSION)).
            arg(QString(QLatin1String(GIT_REVISION)).left(10)).
            arg(QLatin1String("ABBAPOH")).
            arg(QLatin1String("ABBAPOH@nextmail.ru"));

    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("About Andromeda"));
    msgBox.setText(text);
    msgBox.setIconPixmap(QPixmap(":/images/icons/andromeda.png"));
    msgBox.exec();
}

/*!
    Shows "about Qt" dialog.
*/
void Application::aboutQt()
{
    QMessageBox::aboutQt(0);
}

bool Application::eventFilter(QObject *object, QEvent *event)
{
    if (object == settingsWindow) {
        if (event->type() == QEvent::Close) {
            settingsWindowState = settingsWindow->saveState();
        }
    }
    return false;
}

void Application::handleMessage(const QString &message)
{
    QStringList arguments = message.split("\n");
    if (arguments.isEmpty())
        return;

    handleArguments(arguments);
    m_pluginManager->postInitialize(arguments.mid(1));
}

void Application::handleArguments(const QStringList &arguments)
{
    Q_ASSERT(arguments.count() > 0);

    QStringList urls = arguments.mid(1);
    QString currentDir = arguments.first();

    if (!urls.isEmpty()) {
        BrowserWindow *window = new BrowserWindow();
        foreach (const QString &url, urls)
            window->openNewTab(urlFromUserInput(currentDir, url));
        window->show();
        return;
    }

    if (EditorWindow::windows().isEmpty() || !m_firstStart) {
        BrowserWindow::newWindow();
    }

    m_firstStart = false;
}

bool Application::restoreApplicationState(const QByteArray &arr)
{
    QByteArray state = arr;
    QDataStream s(&state, QIODevice::ReadOnly);

    qint32 magic;
    qint8 version;
    quint32 windowCount;
    QByteArray windowState;
    bool trayIconVisible;

    s >> magic;
    if (magic != andromedaAppMagic)
        return false;

    s >> version;
    if (version != andromedaAppVersion)
        return false;

    s >> windowCount;

    if (windowCount == 0)
        newWindow();

    for (quint32 i = 0; i < windowCount; i++) {
        s >> windowState;

        BrowserWindow *window = new BrowserWindow();
        window->show();
        bool ok = window->restoreState(windowState);
        if (!ok)
            return false;
    }

    s >> settingsWindowState;
    s >> trayIconVisible;

    setTrayIconVisible(trayIconVisible);

    return true;
}

QByteArray Application::saveApplicationState() const
{
    QByteArray state;
    QDataStream s(&state, QIODevice::WriteOnly);

    s << andromedaAppMagic;
    s << andromedaAppVersion;

    QList<BrowserWindow*> windows = BrowserWindow::windows();
    quint32 windowCount = windows.count();

    s << windowCount;

    for (quint32 i = 0; i < windowCount; i++) {
        s << windows[i]->saveState();
        // hide window to prevent strange crash when bookmarks editor is opened
        windows[i]->hide();
    }

    s << settingsWindowState;
    s << isTrayIconVisible();

    return state;
}

void Application::loadSettings()
{
    m_settings = new QSettings(this);
    m_settings->beginGroup("mainWindow");
    QByteArray geometry = m_settings->value("geometry").toByteArray();

    if (!geometry.isEmpty())
        BrowserWindow::setWindowGeometry(geometry);
}

void Application::saveSettings()
{
    m_settings->setValue("geometry", BrowserWindow::windowGeometry());
}

void Application::createActions()
{
    MenuBarContainer *menuBar = new MenuBarContainer(this);
    menuBar->createMenus();

    createGoToMenu();
    createToolsMenu();
    createWindowsMenu();
    createDockMenu();
    registerAtions();
}

void Application::createGoToMenu()
{
    MenuBarContainer *menuBar = MenuBarContainer::instance();

    Command *c = 0;

    // ================ GoTo Menu ================
    CommandContainer *goToMenu = new CommandContainer(Constants::Menus::GoTo, this);
    menuBar->addContainer(goToMenu, "025");

    // ================ GoTo Menu (default) ================
    c = new Command(Constants::Actions::Back, this);
    c->setDefaultShortcut(QKeySequence::Back);
    goToMenu->addCommand(c);

    c = new Command(Constants::Actions::Forward, this);
    c->setDefaultShortcut(QKeySequence::Forward);
    goToMenu->addCommand(c);

#ifdef Q_OS_MAC
    c = new Command(Constants::Actions::Up, QKeySequence(QLatin1String("Ctrl+Up")), tr("Up one level"), this);
#else
    c = new Command(Constants::Actions::Up, QKeySequence(QLatin1String("Alt+Up")), tr("Up one level"), this);
#endif
    goToMenu->addCommand(c);
    retranslateGoToMenu();
}

void Application::retranslateGoToMenu()
{
    ActionManager *manager = ActionManager::instance();
    CommandContainer *goToMenu = manager->container(Constants::Menus::GoTo);
    goToMenu->setTitle(tr("Go to"));

    manager->command(Constants::Actions::Back)->setDefaultText(tr("Back"));
    manager->command(Constants::Actions::Forward)->setDefaultText(tr("Forward"));
    manager->command(Constants::Actions::Up)->setDefaultText(tr("Up one level"));
}

void Application::createWindowsMenu()
{
    MenuBarContainer *menuBar = MenuBarContainer::instance();

    CommandContainer *windowsMenu = new WindowsContainer(Constants::Menus::Windows, this);
    windowsMenu->setTitle(tr("Windows"));
    menuBar->addContainer(windowsMenu, "035");
}

void Application::createToolsMenu()
{
#ifdef QT_DEBUG
    MenuBarContainer *menuBar = MenuBarContainer::instance();
#endif

    // ================ Tools Menu ================
#ifdef QT_DEBUG
    CommandContainer *toolsMenu = menuBar->container(MenuBarContainer::ToolsMenu);
#endif

#ifdef QT_DEBUG
    Command *plugins = new Command(Constants::Actions::Plugins, this);
    plugins->setDefaultText(tr("Plugins..."));
    plugins->setContext(Command::ApplicationCommand);
    toolsMenu->addCommand(plugins);
#endif

#ifdef QT_DEBUG
    Command *settings = new Command(Constants::Actions::Settings, this);
    settings->setDefaultText(tr("View all settings..."));
    settings->setContext(Command::ApplicationCommand);
    toolsMenu->addCommand(settings);
#endif
}

#ifdef Q_OS_MAC
void qt_mac_set_dock_menu(QMenu *menu);
#endif

void Application::createDockMenu()
{
    MenuBarContainer *menuBar = MenuBarContainer::instance();

    CommandContainer *dock = new DockContainer(Constants::Menus::Dock, this);
#ifdef Q_OS_MAC
    dock->setTitle(tr("Dock menu"));
#else
    dock->setTitle(tr("Tray menu"));
#endif

    dock->addCommand(menuBar->command(MenuBarContainer::NewWindow));
#ifndef Q_OS_MAC
    dock->addCommand(new Separator(this));
    dock->addCommand(menuBar->command(MenuBarContainer::Quit));
#endif

    dockMenu = dock->menu();

#ifdef Q_OS_MAC
    qt_mac_set_dock_menu(dockMenu);
#endif

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/images/icons/andromeda.png"));
    trayIcon->setContextMenu(dockMenu);
    trayIcon->show();
}

void Application::registerAtions()
{
    createAction(Constants::Actions::NewWindow, SLOT(newWindow()));
    createAction(Constants::Actions::Quit, SLOT(exit()));
    createAction(Constants::Actions::Plugins, SLOT(showPluginView()));
    createAction(Constants::Actions::Settings, SLOT(showSettings()));
    createAction(Constants::Actions::Preferences, SLOT(preferences()));
    createAction(Constants::Actions::About, SLOT(about()));
    createAction(Constants::Actions::AboutQt, SLOT(aboutQt()));
}

void Application::createAction(const QByteArray &id, const char *slot)
{
    Action *action = new Action(id, this);
    connect(action, SIGNAL(triggered()), slot);
}
