#ifndef APPLICATION_H
#define APPLICATION_H

#include <QtSingleApplication>
#include <QPointer>

#ifdef qApp
#undef qApp
#endif
#define qApp Application::instance()

class QMenu;
class QMenuBar;
class QSettings;
class QSystemTrayIcon;

namespace ExtensionSystem {
class PluginManager;
class PluginView;
} // namespace ExtensionSystem

namespace GuiSystem {
class CommandContainer;
class SettingsPageManager;
class SettingsWindow;
} // namespace GuiSystem

namespace Andromeda {

class Application : public QtSingleApplication
{
    Q_OBJECT
    Q_DISABLE_COPY(Application)
public:
    explicit Application(int &argc, char **argv);
    ~Application();

    static Application *instance();

    bool activateApplication();
    bool loadPlugins();

    bool isTrayIconVisible() const;
    void setTrayIconVisible(bool visible);

public slots:
    void newWindow();
    void showPluginView();
    void showSettings();
    void preferences();

    void restoreSession();
    void saveSession();

    void exit();

    void about();
    void aboutQt();

protected:
    bool eventFilter(QObject *object, QEvent *event);

private slots:
    void handleMessage(const QString &message);

private:
    void handleArguments(const QStringList &arguments);

    bool restoreApplicationState(const QByteArray &state);
    QByteArray saveApplicationState() const;
    void loadSettings();
    void saveSettings();

    void createMenus();
    void createFileMenu();
    void createEditMenu();
    void createViewMenu();
    void createGoToMenu();
    void createBookmarksMenu();
    void createPanesMenu();
    void createToolsMenu();
    void createWindowsMenu();
    void createHelpMenu();
    void createDockMenu();
    void setupApplicationActions();

    void createAction(const QByteArray &id, const char *slot);

private:
    ExtensionSystem::PluginManager *m_pluginManager;
    GuiSystem::SettingsPageManager *m_settingsPageManager;

    QPointer<GuiSystem::SettingsWindow> settingsWindow;
    QByteArray settingsWindowState;
    QMenu *dockMenu;
    QSystemTrayIcon *trayIcon;
    QSettings *m_settings;
    bool m_firstStart;
#ifdef Q_OS_MAC
    QMenuBar *macMenuBar;
#endif

    GuiSystem::CommandContainer *menuBar;
};

} // namespace Andromeda

#endif // APPLICATION_H
