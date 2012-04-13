#ifndef COREPLUGIN_H
#define COREPLUGIN_H

#include <extensionsystem/iplugin.h>
#include "browserwindow.h"

#include <QtCore/QPointer>

using namespace ExtensionSystem;

namespace GuiSystem {
class SettingsDialog;
}

class CorePlugin : public IPlugin
{
    Q_OBJECT
public:
    CorePlugin();
    ~CorePlugin();

    bool initialize();
    void postInitialize(const QVariantMap &);
    void shutdown();

    bool restoreState(const QByteArray &state);
    QByteArray saveState() const;
    void loadSettings();
    void saveSettings();

public slots:
    void newWindow();
    void showPluginView();
    void showSettings();
    void prefenrences();

    void restoreSession();
    void saveSession();

    void quit();

    void about();
    void aboutQt();

protected:
    bool eventFilter(QObject *o, QEvent *e);

private:
    void createActions();
    void createFileMenu();
    void createEditMenu();
    void createViewMenu();
    void createGoToMenu();
    void createWindowsMenu();
    void createToolsMenu();
    void createHelpMenu();
    void createDockMenu();
    void registerAtions();

    void createAction(const QByteArray &id, const char *slot);

private:
    QStringList urls;
    QPointer<GuiSystem::SettingsDialog> settingsDialog;
    QByteArray settingsDialogState;
    QMenu *dockMenu;
    QSettings *m_settings;
#ifdef Q_OS_MAC
    QMenuBar *menuBar;
#endif
};

#endif // COREPLUGIN_H
