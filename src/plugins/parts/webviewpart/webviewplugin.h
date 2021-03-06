#ifndef WEBVIEWPLUGINIMPL_H
#define WEBVIEWPLUGINIMPL_H

#include <ExtensionSystem/IPlugin>

#include <QtWebKit/QWebHistoryInterface>

class CookieJar;
class QSettings;
class QWebSettings;

namespace WebView {

class WebViewPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

#if QT_VERSION >= 0x050000
    Q_PLUGIN_METADATA(IID "com.arch.Andromeda.WebViewPlugin")
#endif
public:
    WebViewPlugin();
    ~WebViewPlugin();

    static WebViewPlugin *instance();

    inline CookieJar *cookieJar() const { return m_cookieJar; }

    bool initialize();
    void shutdown();

    QWebSettings* webSettings();

private:
    void createActions();
    void loadSettings();
    void loadAppearanceSettings();
    void loadProxySettings();
    void loadPrivacySettings();
    void saveSettings();
    void saveAppearanceSettings();
    void saveProxySettings();
    void savePrivacySettings();

private:
    CookieJar *m_cookieJar;
    QWebSettings *m_webSettings;
    QSettings *m_settings;

    friend class ProxySettingsWidget;
};

class WebHistoryInterface : public QWebHistoryInterface
{
    Q_OBJECT

public:
    explicit WebHistoryInterface(QObject *parent = 0);
    ~WebHistoryInterface();

    static WebHistoryInterface *instance();

    bool historyContains(const QString &url) const;
    void addHistoryEntry(const QString &url);

signals:
    void itemAdded();
};

} // namespace WebView

#endif // WEBVIEWPLUGINIMPL_H
