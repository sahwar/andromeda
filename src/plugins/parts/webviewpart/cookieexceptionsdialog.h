#ifndef COOKIESEXCEPTIONSDIALOG_H
#define COOKIESEXCEPTIONSDIALOG_H

#include <QtCore/qglobal.h>

#if QT_VERSION >= 0x050000
#include <QtWidgets/QDialog>
#else
#include <QtGui/QDialog>
#endif

class CookieJar;
class CookieExceptionsModel;
class QSortFilterProxyModel;

namespace Ui {
class CookieExceptionsDialog;
} // namespace Ui

class CookieExceptionsDialog : public QDialog
{
    Q_OBJECT
    Q_DISABLE_COPY(CookieExceptionsDialog)

public:
    explicit CookieExceptionsDialog(QWidget *parent = 0);
    ~CookieExceptionsDialog();

    CookieJar *cookieJar() const;
    void setCookieJar(CookieJar *cookieJar);

public slots:
    void block();
    void allow();
    void allowForSession();
    void textChanged(const QString &text);

    void remove();
    void removeAll();

private:
    void adjustColumns();

private:
    Ui::CookieExceptionsDialog *ui;

    CookieExceptionsModel *m_exceptionsModel;
    QSortFilterProxyModel *m_proxyModel;
    CookieJar *m_cookieJar;
};

#endif // COOKIESEXCEPTIONSDIALOG_H
