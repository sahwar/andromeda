#ifndef ABSTRACTEDITORFACTORY_H
#define ABSTRACTEDITORFACTORY_H

#include "guisystem_global.h"

#include "abstractviewfactory.h"

#include <QtCore/QObject>

class QWidget;

namespace GuiSystem {

class AbstractEditor;

class GUISYSTEM_EXPORT AbstractEditorFactory : public AbstractViewFactory
{
    Q_OBJECT
    Q_DISABLE_COPY(AbstractEditorFactory)

public:
    explicit AbstractEditorFactory(QObject *parent = 0);
    ~AbstractEditorFactory();

    AbstractEditor *editor(QWidget *parent);

    virtual QStringList mimeTypes() = 0;

protected:
    virtual AbstractEditor *createEditor(QWidget *parent) = 0;
    AbstractView * createView(QWidget *parent);

    friend class AbstractEditor;
};

} // namespace GuiSystem

#endif // ABSTRACTEDITORFACTORY_H