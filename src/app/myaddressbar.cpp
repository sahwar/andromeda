#include "myaddressbar.h"

#if QT_VERSION >= 0x050000
#include <QtWidgets/QAction>
#else
#include <QtGui/QAction>
#endif

#include <Parts/constants.h>

using namespace Andromeda;

/*!
    \class Andromeda::MyAddressBar
    Addressbar with actions connected to menu via Parts::ActionManager.
*/

/*!
    Creates MyAddressBar with the given \a parent.
*/
MyAddressBar::MyAddressBar(QWidget *parent) :
    AddressBar(parent)
{
    createActions();
    retranslateUi();
}

/*!
    \internal
*/
void MyAddressBar::createActions()
{
    actions[Redo] = new QAction(this);
    actions[Redo]->setObjectName(Constants::Actions::Redo);
    connect(actions[Redo], SIGNAL(triggered()), SLOT(redo()));

    actions[Undo] = new QAction(this);
    actions[Undo]->setObjectName(Constants::Actions::Undo);
    connect(actions[Undo], SIGNAL(triggered()), SLOT(undo()));

    actions[Cut] = new QAction(this);
    actions[Cut]->setObjectName(Constants::Actions::Cut);
    connect(actions[Cut], SIGNAL(triggered()), SLOT(cut()));

    actions[Copy] = new QAction(this);
    actions[Copy]->setObjectName(Constants::Actions::Copy);
    connect(actions[Copy], SIGNAL(triggered()), SLOT(copy()));

    actions[Paste] = new QAction(this);
    actions[Paste]->setObjectName(Constants::Actions::Paste);
    connect(actions[Paste], SIGNAL(triggered()), SLOT(paste()));

    actions[SelectAll] = new QAction(this);
    actions[SelectAll]->setObjectName(Constants::Actions::SelectAll);
    connect(actions[SelectAll], SIGNAL(triggered()), SLOT(selectAll()));

    for (int i = 0; i < ActionCount; i++) {
        actions[SelectAll]->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        addAction(actions[i]);
    }
}

/*!
    \internal
*/
void MyAddressBar::retranslateUi()
{
    actions[Redo]->setText(tr("Redo"));
    actions[Undo]->setText(tr("Undo"));

    actions[Cut]->setText(tr("Cut"));
    actions[Copy]->setText(tr("Copy"));
    actions[Paste]->setText(tr("Paste"));
    actions[SelectAll]->setText(tr("Select all"));
}
