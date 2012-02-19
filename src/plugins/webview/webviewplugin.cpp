#include "webviewplugin.h"

#include <QtCore/QtPlugin>
#include <QtGui/QFileIconProvider>

#include <guisystem/editormanager.h>
#include <guisystem/settingspagemanager.h>

#include "appearancesettings.h"
#include "cookiejar.h"
#include "privacysettings.h"
#include "proxysettings.h"
#include "webvieweditor.h"
#include "webviewsettingspage.h"

#include <QtCore/QSettings>
#include <QtCore/QVariant>
#include <QtNetwork/QNetworkProxy>
#include <QtWebKit/QWebSettings>

using namespace GuiSystem;
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

bool WebViewPlugin::initialize(const QVariantMap &)
{
    SettingsPageManager *pageManager = object<SettingsPageManager>("settingsPageManager");

    pageManager->addPage(new WebViewSettingsPage(this));
    pageManager->addPage(new AppearanceSettingsPage(this));
    pageManager->addPage(new PrivacySettingsPage);
    pageManager->addPage(new ProxySettingsPage(this));

    EditorManager::instance()->addFactory(new WebViewEditorFactory(this));

    m_cookieJar = new CookieJar(this);

    loadSettings();

    return true;
}

void WebViewPlugin::shutdown()
{
}

void WebViewPlugin::loadSettings()
{
    m_settings = new QSettings(this);
    m_webSettings = QWebSettings::globalSettings();

    m_settings->beginGroup(QLatin1String("webview"));

    loadAppearanceSettings();
    loadProxySettings();
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

    fixedFont = qVariantValue<QFont>(m_settings->value(QLatin1String("fixedFont"), fixedFont));
    standardFont = qVariantValue<QFont>(m_settings->value(QLatin1String("standardFont"), standardFont));
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

Q_EXPORT_PLUGIN(WebViewPlugin)