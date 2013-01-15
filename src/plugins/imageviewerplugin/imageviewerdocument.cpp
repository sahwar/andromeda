#include "imageviewerdocument.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include <ImageView/ImageView>

#include "imageviewereditor.h"

using namespace GuiSystem;
using namespace ImageViewer;

ImageViewerDocument::ImageViewerDocument(QObject *parent) :
    FileDocument(parent)
{
    setIcon(QIcon(":/icons/imageview.png"));
}

bool ImageViewerDocument::read(QIODevice *device, const QString &fileName)
{
    editor->m_view->read(device, QFileInfo(fileName).suffix().toUtf8());
    return true;
}

bool ImageViewerDocument::write(QIODevice *device, const QString &fileName)
{
    editor->m_view->write(device, QFileInfo(fileName).suffix().toUtf8());
    return true;
}

/*!
    \class ImageViewerDocumentFactory
*/

/*!
    Creates ImageViewerDocumentFactory with the given \a parent.
*/
ImageViewerDocumentFactory::ImageViewerDocumentFactory(QObject *parent)
{
}

/*!
    \reimp
*/
QByteArray ImageViewerDocumentFactory::id() const
{
    return "imageview";
}

/*!
    \reimp
*/
QString ImageViewerDocumentFactory::name() const
{
    return tr("Image viewer");
}

/*!
    \reimp
*/
QIcon ImageViewerDocumentFactory::icon() const
{
    return QIcon(":/icons/imageview.png");
}

/*!
    \reimp
*/
QStringList ImageViewerDocumentFactory::mimeTypes() const
{
    return QStringList() << "image/jpeg" << "image/png";
}

/*!
    \reimp
*/
AbstractDocument * ImageViewerDocumentFactory::createDocument(QObject *parent)
{
    return new ImageViewerDocument(parent);
}
