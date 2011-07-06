#ifndef COMMANDCONTAINER_H
#define COMMANDCONTAINER_H

#include <QtCore/QObject>

class QMenu;
class QMenuBar;
class QToolBar;

namespace GuiSystem {

class Command;
class CommandContainerPrivate;
class CommandContainer : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(CommandContainer)
    Q_DISABLE_COPY(CommandContainer)
public:
    explicit CommandContainer(const QString &id, QObject *parent = 0);
    ~CommandContainer();

    void addCommand(Command *command);
    void addContainer(CommandContainer *container);
    void addSeparator();

    void clear();

    QString id() const;

    QMenu *menu() const;
    QMenuBar *menuBar() const;
    QToolBar *toolBar() const;

    QString title() const;
    void setTitle(const QString &title);

protected:
    CommandContainerPrivate *d_ptr;
};

} // namespace GuiSystem

#endif // COMMANDCONTAINER_H
