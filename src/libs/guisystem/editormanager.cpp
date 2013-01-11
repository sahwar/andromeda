#include "editormanager.h"

#include "abstracteditorfactory.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QStringList>

#include <IO/QMimeDatabase>

namespace GuiSystem {

class EditorManagerPrivate
{
public:
    QHash<QString, AbstractEditorFactory *> factoriesForId;
    QHash<QString, EditorManager::FactoryList> factoriesForMimeType;
    QHash<QString, EditorManager::FactoryList> factoriesForScheme;
};

} // namespace GuiSystem

using namespace GuiSystem;

static inline bool editorFactoryLessThan(AbstractEditorFactory *first, AbstractEditorFactory *second)
{
    return first->weight() > second->weight();
}

/*!
  \class GuiSystem::EditorManager

  \brief The EditorManager class contains all editors' and views' factories.
*/

/*!
  \brief Creates a EditorManager with the given \a parent.
*/
EditorManager::EditorManager(QObject *parent) :
    QObject(parent),
    d_ptr(new EditorManagerPrivate)
{
}

/*!
  \brief Destroys EditorManager.
*/
EditorManager::~EditorManager()
{
    delete d_ptr;
}

Q_GLOBAL_STATIC(EditorManager, staticInstance)

/*!
  \brief Returns static instance of an editor.
*/
EditorManager * EditorManager::instance()
{
    return staticInstance();
}

/*!
  \brief Returns factory with given \a id.
*/
AbstractEditorFactory * EditorManager::factoryForId(const QString &id) const
{
    return d_func()->factoriesForId.value(id);
}

/*!
  \brief Returns factory that can handle given \a mimeType.
*/
AbstractEditorFactory * EditorManager::factoryForMimeType(const QString &mimeType) const
{
    const FactoryList &factories = factoriesForMimeType(mimeType);
    if (factories.isEmpty())
        return 0;

    return factories.first();
}

/*!
  \brief Returns factory that can handle given \a scheme.
*/
AbstractEditorFactory * EditorManager::factoryForScheme(const QString &scheme) const
{
    const FactoryList &factories = factoriesForScheme(scheme);
    if (factories.isEmpty())
        return 0;

    return factories.first();
}

/*!
  \brief Returns factory that can handle given \a url.
*/
AbstractEditorFactory * EditorManager::factoryForUrl(const QUrl &url) const
{
    const FactoryList &factories = this->factoriesForUrl(url);
    if (factories.isEmpty())
        return 0;

    return factories.first();
}

/*!
  \brief Returns list of all factories.
*/
EditorManager::FactoryList EditorManager::factories() const
{
    return d_func()->factoriesForId.values();
}

/*!
  \brief Returns list of factories that can handle given \a mimeType.
*/
EditorManager::FactoryList EditorManager::factoriesForMimeType(const QString &mimeType) const
{
    FactoryList result;

    QMimeDatabase db;
    QMimeType mt = db.mimeTypeForName(mimeType);
    QStringList mimeTypes;
    mimeTypes.append(mimeType);
    mimeTypes.append(mt.parentMimeTypes());
    foreach (const QString &mimeType, mimeTypes) {
        const FactoryList &factories = d_func()->factoriesForMimeType.value(mimeType);
        foreach (AbstractEditorFactory *f, factories) {
            if (!result.contains(f))
                result.append(f);
        }
    }

    qStableSort(result.begin(), result.end(), editorFactoryLessThan);

    return result;
}

/*!
  \brief Returns list of factories that can handle given \a scheme.
*/
EditorManager::FactoryList EditorManager::factoriesForScheme(const QString &scheme) const
{
    return d_func()->factoriesForScheme.value(scheme);
}

/*!
  \brief Returns list of factories that can handle given \a url.
*/
EditorManager::FactoryList EditorManager::factoriesForUrl(const QUrl &url) const
{
    QList<AbstractEditorFactory *> result;

    if (url.scheme() == qApp->applicationName()) {
        result.append(factoryForId(url.host()));
    } else {
        QMimeDatabase db;
        QList<QMimeType> mimeTypes;

        mimeTypes.append(db.mimeTypesForFileName(QFileInfo(url.path()).fileName()));
        mimeTypes.append(db.mimeTypeForUrl(url));
        foreach (const QMimeType &mimeType, mimeTypes) {
            QString mimeTypeName = mimeType.name();
            result.append(factoriesForMimeType(mimeTypeName));
        }

        // add scheme factory last to maximize chance to
        // get "mime type editor" with same weight instead of
        // "scheme editor"
        result.append(factoriesForScheme(url.scheme()));
    }

    result.erase(std::unique(result.begin(), result.end()), result.end());
    qStableSort(result.begin(), result.end(), editorFactoryLessThan);

    return result;
}

EditorManager::FactoryList EditorManager::factoriesForUrls(const QList<QUrl> &urls) const
{
    if (urls.isEmpty())
        return FactoryList();

    FactoryList factories = this->factoriesForUrl(urls.first());

    foreach (const QUrl &url, urls.mid(1)) {
        if (factories.isEmpty())
            break;

        FactoryList list = this->factoriesForUrl(url);
        foreach (AbstractEditorFactory *f, factories) {
            if (!list.contains(f))
                factories.removeOne(f);
        }
    }

    return factories;
}

/*!
  \brief Adds \a factory to EditorManager.

  Factories automatically removed when being destroyed.
*/
void EditorManager::addFactory(AbstractEditorFactory *factory)
{
    Q_D(EditorManager);

    if (!factory)
        return;

    if (d->factoriesForId.contains(factory->id())) {
        qWarning() << "EditorManager::addFactory :"
                   << QString("Factory with id %1 already exists").arg(QString(factory->id()));
        return;
    }

    d->factoriesForId.insert(factory->id(), factory);

    foreach (const QString &mimeType, factory->mimeTypes()) {
        d->factoriesForMimeType[mimeType].append(factory);
    }

    foreach (const QString &scheme, factory->urlSchemes()) {
        d->factoriesForScheme[scheme].append(factory);
    }

    connect(factory, SIGNAL(destroyed(QObject *)), this, SLOT(onDestroyed1(QObject*)));
}

static void removeFactory(QHash<QString, EditorManager::FactoryList> &hash, AbstractEditorFactory *factory)
{
    QMutableHashIterator<QString, EditorManager::FactoryList> i(hash);
    while (i.hasNext()) {
        i.next();
        EditorManager::FactoryList &factories = i.value();
        factories.removeAll(factory);
        if (factories.isEmpty())
            i.remove();
    }
}

/*!
  \brief Removes \a factory from EditorManager.
*/
void EditorManager::removeFactory(AbstractEditorFactory *factory)
{
    Q_D(EditorManager);

    if (!factory)
        return;

    ::removeFactory(d->factoriesForMimeType, factory);
    ::removeFactory(d->factoriesForScheme, factory);

    d->factoriesForId.remove(d->factoriesForId.key(factory));

    disconnect(factory, 0, this, 0);
}

/*!
  \internal
*/
void EditorManager::onDestroyed1(QObject *o)
{
    removeFactory(static_cast<AbstractEditorFactory *>(o));
}

/*!
  \internal
*/
QString EditorManager::getMimeType(const QUrl &url) const
{
    if (url.scheme() == QLatin1String("file")) {
        QFileInfo info(url.toLocalFile());
        if (info.isDir() && !info.isBundle())
            return QLatin1String("inode/directory");
    } else if(url.scheme() == QLatin1String("http") ||
              url.scheme() == QLatin1String("https")) {
        return QLatin1String("text/html");
    }
    return QString();
}

