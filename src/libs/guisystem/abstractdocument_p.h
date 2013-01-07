#ifndef ABSTRACTDOCUMENT_P_H
#define ABSTRACTDOCUMENT_P_H

#include "abstractdocument.h"

namespace GuiSystem {

class AbstractDocumentPrivate
{
    Q_DECLARE_PUBLIC(AbstractDocument)
    Q_DISABLE_COPY(AbstractDocumentPrivate)
public:
    explicit AbstractDocumentPrivate(AbstractDocument *qq) :
        modified(false),
        progress(0),
        readOnly(true),
        state(AbstractDocument::NoState),
        q_ptr(qq)
    {}

    QIcon icon;
    bool modified;
    int progress;
    bool readOnly;
    AbstractDocument::State state;
    QString title;
    QUrl url;

private:
    AbstractDocument *q_ptr;
};

} // namespace GuiSystem

#endif // ABSTRACTDOCUMENT_P_H