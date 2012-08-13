#include "commandssettingspage.h"

#include <guisystem/commandssettingswidget.h>

using namespace GuiSystem;
using namespace Core;

/*!
    \class Core::CommandsSettingsPage

    \brief Settings page for editing shortcuts.

    \sa GuiSystem::CommandsSettingsWidget.
*/

/*!
    Creates CommandsSettingsPage with the given \a parent.
*/
CommandsSettingsPage::CommandsSettingsPage(QObject *parent) :
    SettingsPage(QLatin1String("CommandSettings"), QLatin1String(" General"), parent)
{
}

/*!
    \reimp
*/
QString CommandsSettingsPage::name() const
{
    return tr("Shortcut settings");
}

/*!
    \reimp
*/
QIcon CommandsSettingsPage::icon() const
{
    return QIcon();
}

/*!
    \reimp
*/
QString CommandsSettingsPage::categoryName() const
{
    return tr("General");
}

/*!
    \reimp
*/
QIcon CommandsSettingsPage::categoryIcon() const
{
    return QIcon(":/images/icons/andromeda.png");
}

/*!
    \reimp
*/
QWidget * CommandsSettingsPage::createPage(QWidget *parent)
{
    return new CommandsSettingsWidget(parent);
}
