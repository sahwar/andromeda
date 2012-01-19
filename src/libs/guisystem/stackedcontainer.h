#ifndef STACKEDCONTAINER_H
#define STACKEDCONTAINER_H

#include "guisystem_global.h"

#include "abstractcontainer.h"

namespace GuiSystem {

class StackedContainerPrivate;
class GUISYSTEM_EXPORT StackedContainer : public AbstractContainer
{
    Q_OBJECT
    Q_DISABLE_COPY(StackedContainer)

public:
    explicit StackedContainer(QWidget *parent = 0);
    ~StackedContainer();

    Capabilities capabilities() const;

    int count() const;
    int currentIndex() const;
    AbstractEditor *editor(int index) const;

    bool isModified() const;
    bool isReadOnly() const;

    QUrl url() const;

    AbstractHistory *history() const;

    virtual void restoreState(const QByteArray &state);
    virtual QByteArray saveState() const;

public slots:
    void open(const QUrl &url = QUrl());
    void openNewEditor(const QUrl &url);

    void setCurrentIndex(int index);

private slots:
    void onUrlChanged(const QUrl &url);
    void onIndexChanged(int index);
    void onDestroy(QObject*);

private:
    StackedContainerPrivate *d;

    friend class StackedContainerPrivate;
};

} // namespace GuiSystem

#endif // STACKEDCONTAINER_H