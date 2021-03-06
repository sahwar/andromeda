#include "webviewplugin.h"

#include <QtCore/QtPlugin>
#include <QtCore/QSettings>
#include <QtCore/QVariant>

#if QT_VERSION >= 0x050000
#include <QtWidgets/QFileIconProvider>
#else
#include <QtGui/QFileIconProvider>
#endif

#include <QtNetwork/QNetworkProxy>
#include <QtWebKit/QWebSettings>

#include <Parts/ActionManager>
#include <Parts/ContextCommand>
#include <Parts/CommandContainer>
#include <Parts/DocumentManager>
#include <Parts/EditorManager>
#include <Parts/SettingsPageManager>
#include <Parts/constants.h>

#include "appearancesettings.h"
#include "cookiejar.h"
#include "privacysettings.h"
#include "proxysettings.h"
#include "webviewconstants.h"
#include "webviewdocument.h"
#include "webvieweditor.h"

using namespace Parts;
using namespace WebView;

WebViewPlugin *m_instance = 0;

WebViewPlugin::WebViewPlugin() :
    ExtensionSystem::IPlugin()
{
    m_instance = this;
}

WebViewPlugin::~WebViewPlugin()
{
    m_instance = 0;
}

WebViewPlugin * WebViewPlugin::instance()
{
    Q_ASSERT_X(m_instance, "WebViewPluginImpl::instance", "Web view plugin is not loaded");
    return m_instance;
}

bool WebViewPlugin::initialize()
{
    SettingsPageManager *pageManager = object<SettingsPageManager>("settingsPageManager");

    pageManager->addPage(new AppearanceSettingsPage(this));
    pageManager->addPage(new PrivacySettingsPage(this));
    pageManager->addPage(new ProxySettingsPage(this));

    DocumentManager::instance()->addFactory(new WebViewDocumentFactory(this));
    EditorManager::instance()->addFactory(new WebViewEditorFactory(this));

    m_cookieJar = new CookieJar(this);

    WebHistoryInterface *iface = new WebHistoryInterface(this);

    if (!QWebHistoryInterface::defaultInterface())
        QWebHistoryInterface::setDefaultInterface(iface);

    createActions();
    loadSettings();

    return true;
}

void WebViewPlugin::shutdown()
{
    saveSettings();
}

void WebViewPlugin::saveSettings()
{
    saveAppearanceSettings();
    saveProxySettings();
    savePrivacySettings();
}

void WebViewPlugin::saveAppearanceSettings()
{
    m_settings->beginGroup(QLatin1String("appearance"));

    QString fixedFontFamily = m_webSettings->fontFamily(QWebSettings::FixedFont);
    int fixedFontSize = m_webSettings->fontSize(QWebSettings::DefaultFixedFontSize);

    QString standardFontFamily = m_webSettings->fontFamily(QWebSettings::StandardFont);
    int standardFontSize = m_webSettings->fontSize(QWebSettings::DefaultFontSize);

    QFont fixedFont = QFont(fixedFontFamily, fixedFontSize);
    QFont standardFont = QFont(standardFontFamily, standardFontSize);

    int minimumFontSize = m_webSettings->fontSize(QWebSettings::MinimumFontSize);
    QVariant defaultEncoding = m_webSettings->defaultTextEncoding();

    m_settings->setValue(QLatin1String("fixedFont"), fixedFont);
    m_settings->setValue(QLatin1String("standardFont"), standardFont);
    m_settings->setValue(QLatin1String("minimumFontSize"), minimumFontSize);
    m_settings->setValue(QLatin1String("defaultEncoding"), defaultEncoding.toByteArray());

    m_settings->endGroup();
}

void WebViewPlugin::savePrivacySettings()
{
    bool blockPopupWindows = false;
    bool enableImages = false;
    bool enableJavascript = false;
    bool enableLocalStorage = false;
    bool enablePlugins = false;

    m_settings->beginGroup(QLatin1String("privacy"));

    if (m_webSettings->testAttribute(QWebSettings::JavascriptCanOpenWindows))
        blockPopupWindows = true;
    if (m_webSettings->testAttribute(QWebSettings::AutoLoadImages))
        enableImages = true;
    if (m_webSettings->testAttribute(QWebSettings::JavascriptEnabled))
        enableJavascript = true;
    if (m_webSettings->testAttribute(QWebSettings::LocalStorageEnabled))
        enableLocalStorage = true;
    if (m_webSettings->testAttribute(QWebSettings::PluginsEnabled))
        enablePlugins = true;

    m_settings->setValue(QLatin1String("blockPopupWindows"), blockPopupWindows);
    m_settings->setValue(QLatin1String("enableImages"), enableImages);
    m_settings->setValue(QLatin1String("javascriptEnabled"), enableJavascript);
    m_settings->setValue(QLatin1String("enableLocalStorage"), enableLocalStorage);
    m_settings->setValue(QLatin1String("enablePlugins"), enablePlugins);

    m_settings->endGroup();
}

void WebViewPlugin::saveProxySettings()
{
    bool enabled = false;
    QNetworkProxy proxy = QNetworkProxy::applicationProxy();

    if (proxy.type() != QNetworkProxy::NoProxy)
        enabled = true;

    m_settings->beginGroup(QLatin1String("proxy"));

    m_settings->setValue(QLatin1String("enabled"), enabled);
    m_settings->setValue(QLatin1String("type"), proxy.type());
    m_settings->setValue(QLatin1String("hostName"), proxy.hostName());
    m_settings->setValue(QLatin1String("port"), proxy.port());
    m_settings->setValue(QLatin1String("userName"), proxy.user());
    m_settings->setValue(QLatin1String("password"), proxy.password());

    m_settings->endGroup();
}

void WebViewPlugin::loadSettings()
{
    m_settings = new QSettings(this);
    m_webSettings = QWebSettings::globalSettings();

    m_settings->beginGroup(QLatin1String("webview"));

    loadAppearanceSettings();
    loadProxySettings();
    loadPrivacySettings();
}

void WebViewPlugin::loadAppearanceSettings()
{
    m_settings->beginGroup(QLatin1String("appearance"));

    QString fixedFontFamily = m_webSettings->fontFamily(QWebSettings::FixedFont);
    int fixedFontSize = m_webSettings->fontSize(QWebSettings::DefaultFixedFontSize);

    QString standardFontFamily = m_webSettings->fontFamily(QWebSettings::StandardFont);
    int standardFontSize = m_webSettings->fontSize(QWebSettings::DefaultFontSize);

    QFont fixedFont = QFont(fixedFontFamily, fixedFontSize);
    QFont standardFont = QFont(standardFontFamily, standardFontSize);

    int minimumFontSize = m_webSettings->fontSize(QWebSettings::MinimumFontSize);
    QByteArray defaultEncoding;

    fixedFont = m_settings->value(QLatin1String("fixedFont"), fixedFont).value<QFont>();
    standardFont = m_settings->value(QLatin1String("standardFont"), standardFont).value<QFont>();
    minimumFontSize = m_settings->value(QLatin1String("minimumFontSize"), minimumFontSize).toInt();
    defaultEncoding = m_settings->value(QLatin1String("defaultEncoding"), "UTF-8").toByteArray();

    m_webSettings->setFontFamily(QWebSettings::FixedFont, fixedFont.family());
    m_webSettings->setFontSize(QWebSettings::DefaultFixedFontSize, fixedFont.pointSize());

    m_webSettings->setFontFamily(QWebSettings::StandardFont, standardFont.family());
    m_webSettings->setFontSize(QWebSettings::DefaultFontSize, standardFont.pointSize());

    if (minimumFontSize)
        m_webSettings->setFontSize(QWebSettings::MinimumFontSize, minimumFontSize);

    m_webSettings->setDefaultTextEncoding(defaultEncoding);

    m_settings->endGroup();
}

void WebViewPlugin::loadProxySettings()
{
    m_settings->beginGroup(QLatin1String("proxy"));

    QNetworkProxy proxy;
    if (m_settings->value(QLatin1String("enabled"), false).toBool()) {
        int proxyType = m_settings->value(QLatin1String("type"), 0).toInt();
        if (proxyType == 0)
            proxy = QNetworkProxy::Socks5Proxy;
        else if (proxyType == 1)
            proxy = QNetworkProxy::HttpProxy;
        else { // 2
            proxy.setType(QNetworkProxy::HttpCachingProxy);
            proxy.setCapabilities(QNetworkProxy::CachingCapability | QNetworkProxy::HostNameLookupCapability);
        }
        proxy.setHostName(m_settings->value(QLatin1String("hostName")).toString());
        proxy.setPort(m_settings->value(QLatin1String("port"), 1080).toInt());
        proxy.setUser(m_settings->value(QLatin1String("userName")).toString());
        proxy.setPassword(m_settings->value(QLatin1String("password")).toString());
    }
    QNetworkProxy::setApplicationProxy(proxy);

    m_settings->endGroup();
}

void WebViewPlugin::loadPrivacySettings()
{
    m_settings->beginGroup(QLatin1String("privacy"));

    bool blockPopupWindows = m_settings->value(QLatin1String("blockPopupWindows"), true).toBool();
    bool enableImages = m_settings->value(QLatin1String("enableImages"), true).toBool();
    bool enableJavascript = m_settings->value(QLatin1String("javascriptEnabled"), true).toBool();
    bool enableLocalStorage = m_settings->value(QLatin1String("enableLocalStorage"), true).toBool();
    bool enablePlugins = m_settings->value(QLatin1String("enablePlugins"), true).toBool();

    m_webSettings->setAttribute(QWebSettings::JavascriptCanOpenWindows, blockPopupWindows);
    m_webSettings->setAttribute(QWebSettings::AutoLoadImages, enableImages);
    m_webSettings->setAttribute(QWebSettings::JavascriptEnabled, enableJavascript);
    m_webSettings->setAttribute(QWebSettings::LocalStorageEnabled, enableLocalStorage);
    m_webSettings->setAttribute(QWebSettings::PluginsEnabled, enablePlugins);

    m_settings->endGroup();
}

QWebSettings* WebViewPlugin::webSettings()
{
    return m_webSettings;
}

void WebViewPlugin::createActions()
{
    ContextCommand *inspectorCommand = new ContextCommand(Constants::Actions::ShowWebInspector, this);
    inspectorCommand->setText(tr("Show web inspector"));
    inspectorCommand->setDefaultShortcut(QKeySequence("Ctrl+Alt+I"));
}

WebHistoryInterface *staticHistory = 0;
WebHistoryInterface::WebHistoryInterface(QObject *parent) :
    QWebHistoryInterface(parent)
{
    Q_ASSERT(!staticHistory);
    ::staticHistory = this;
}

WebHistoryInterface::~WebHistoryInterface()
{
    ::staticHistory = 0;
}

WebHistoryInterface * WebHistoryInterface::instance()
{
    return staticHistory;
}

bool WebHistoryInterface::historyContains(const QString &/*url*/) const
{
    return false;
}

void WebHistoryInterface::addHistoryEntry(const QString &/*url*/)
{
    emit itemAdded();
}

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN(WebViewPlugin)
#endif
