#ifndef STATE_H
#define STATE_H

#include <QtCore/QObject>

namespace GuiSystem {

class PerspectiveInstance;

class StatePrivate;
class State : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(State)
public:
    explicit State(QObject *parent = 0);
    ~State();

    PerspectiveInstance *currentInstance() const;

    void setCurrentPerspectiveId(const QString &id);
//    void closeInstance(const QString &id);

    QStringList perspectiveIds() const;

signals:

public slots:

protected:
    StatePrivate *d_ptr;
};

} // namespace GuiSystem

#endif // STATE_H
