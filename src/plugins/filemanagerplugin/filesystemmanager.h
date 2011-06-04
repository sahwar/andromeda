#ifndef FILESYSTEMMANAGER_H
#define FILESYSTEMMANAGER_H

#include <QtCore/QObject>

class QUndoStack;
class QtFileCopier;

namespace FileManagerPlugin {

class FileSystemManagerPrivate;
class FileSystemManager : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(FileSystemManager)
    Q_DISABLE_COPY(FileSystemManager)
public:
    explicit FileSystemManager(QObject *parent = 0);
    ~FileSystemManager();

    static FileSystemManager *instance();

    QUndoStack *undoStack() const;

    void copyFiles(const QStringList &files, const QString &destination);
    void moveFiles(const QStringList &files, const QString &destination);
    void linkFiles(const QStringList &files, const QStringList &links);

signals:
    void operationStarted(QtFileCopier *);

public slots:

protected:
    FileSystemManagerPrivate *d_ptr;
};

} // namespace FileManagerPlugin

#endif // FILESYSTEMMANAGER_H
