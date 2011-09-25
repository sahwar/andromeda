#ifndef TAB_H
#define TAB_H

#include "coreplugin_global.h"

#include <QtGui/QWidget>

#include "history.h"

class QSettings;

namespace CorePlugin {

class TabPrivate;
class COREPLUGIN_EXPORT Tab : public QWidget
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(Tab)
    Q_DISABLE_COPY(Tab)

    Q_PROPERTY(QString currentPath READ currentPath WRITE open NOTIFY currentPathChanged)

public:
    explicit Tab(QWidget *parent = 0);
    ~Tab();

    void open(const QString &path);

    QString currentPath() const;
    QIcon icon() const;
    QString title() const;
    QString windowTitle() const;

    History *history() const;

    void restoreSession(QSettings &s);
    void saveSession(QSettings &s);

signals:
    void currentPathChanged(const QString & currentPath);
    void changed();

private slots:
    void onIndexChanged(int index);
    void onPathChanged(const QString &path);

protected:
//    void resizeEvent(QResizeEvent *);

protected:
    TabPrivate *d_ptr;
};

} // namespace CorePlugin

#endif // TAB_H
