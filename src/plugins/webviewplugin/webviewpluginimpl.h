#ifndef WEBVIEWPLUGINIMPL_H
#define WEBVIEWPLUGINIMPL_H

#include <iplugin.h>

class WebViewPluginImpl : public ExtensionSystem::IPlugin
{
    Q_OBJECT
public:
    WebViewPluginImpl();

    bool initialize();
    void shutdown();
};

#endif // WEBVIEWPLUGINIMPL_H
