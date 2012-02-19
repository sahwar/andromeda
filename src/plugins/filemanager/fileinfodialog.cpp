#include "fileinfodialog.h"
#include "ui_fileinfodialog.h"

#include <io/directorydetails.h>
#include <widgets/outlinewidget.h>

#include <QtCore/QDateTime>
#include <QtGui/QFileIconProvider>
#include <QtGui/QLineEdit>
#include <io/QDriveInfo>

namespace FileManager {

class LabelLineEdit : public QLineEdit
{
public:
    explicit LabelLineEdit(QWidget *parent = 0);
};

class FileInfoDialogPrivate
{
public:
    explicit FileInfoDialogPrivate(FileInfoDialog *qq) : q(qq) {}

    void updateUi();
    void setupUi();
    void retranslateUi();

public:
    FileInfoDialog *q;

    QGridLayout *topLayout;
    OutlineWidget *widget;
    QVBoxLayout *layout;
    QLabel *iconLabel;
    QLabel *nameLabel;
    QWidget *widget_1;
    QWidget *widget_2;
    QWidget *widget_3;
    QFormLayout *layout_1;
    QFormLayout *layout_2;
    QFormLayout *layout_3;
    QLabel *sizeLabel;
    QLabel *size;
    QLabel *locationLabel;
    QLineEdit *location;
    QLabel *createdLabel;
    QLabel *created;
    QLabel *modifiedLabel;
    QLabel *modified;
    QLabel *acceccedLabel;
    QLabel *accecced;

    QLabel *driveLabel;
    QLabel *drive;
    QLabel *totalSizeLabel;
    QLabel *totalSize;
    QLabel *availableSizeLabel;
    QLabel *availableSize;
    QLabel *mountPointLabel;
    QLineEdit *mountPoint;
    QLabel *fileSystemLabel;
    QLabel *fileSystem;

    QComboBox *userPermissionsComboBox;
    QComboBox *groupPermissionsComboBox;
    QComboBox *otherPermissionsComboBox;

    QFileInfo fileInfo;
    QDriveInfo driveInfo;
    DirectoryDetails *directoryDetails;
};

} // namespace FileManager

using namespace FileManager;

static QString sizeToString(qint64 size)
{
    if (size > (qint64)1024*1024*1024*1024)
        return QObject::tr("%1 TB").arg(size/((qint64)1024*1024*1024*1024));
    if (size > 1024*1024*1024)
        return QObject::tr("%1 GB").arg(size/(1024*1024*1024));
    if (size > 1024*1024)
        return QObject::tr("%1 MB").arg(size/(1024*1024));
    if (size > 1024)
        return QObject::tr("%1 kB").arg(size/1024);
    return QObject::tr("%1 b").arg(size);
}

LabelLineEdit::LabelLineEdit(QWidget *parent) :
    QLineEdit(parent)
{
    QPalette p = palette();
    p.setColor(QPalette::Base, p.color(QPalette::Window));
    setPalette(p);

    setFrame(false);
    setReadOnly(true);
}

void FileInfoDialogPrivate::updateUi()
{
    QIcon icon = QFileIconProvider().icon(fileInfo);
    q->setWindowIcon(icon);
    q->setWindowTitle(QObject::tr("\"%1\" info").arg(fileInfo.fileName()));

    iconLabel->setPixmap(icon.pixmap(32));
    if (fileInfo.isDir())
        sizeLabel->setText(QObject::tr("Calculating..."));
    else
        nameLabel->setText(fileInfo.fileName());

    size->setText(sizeToString(fileInfo.size()));
    location->setText(fileInfo.path());
    created->setText(fileInfo.created().toString(Qt::SystemLocaleShortDate));
    modified->setText(fileInfo.lastModified().toString(Qt::SystemLocaleShortDate));
    accecced->setText(fileInfo.lastRead().toString(Qt::SystemLocaleShortDate));

    drive->setText(driveInfo.name());
    mountPoint->setText(driveInfo.rootPath());
    fileSystem->setText(driveInfo.fileSystemName());
    totalSize->setText(sizeToString(driveInfo.bytesTotal()));
    availableSize->setText(sizeToString(driveInfo.bytesAvailable()));

    userPermissionsComboBox->setCurrentIndex((fileInfo.permissions() & QFile::WriteOwner) ? 1 : 0);
    groupPermissionsComboBox->setCurrentIndex((fileInfo.permissions() & QFile::WriteGroup) ? 1 : 0);
    otherPermissionsComboBox->setCurrentIndex((fileInfo.permissions() & QFile::WriteOther) ? 1 : 0);
}

void FileInfoDialogPrivate::setupUi()
{
    q->setWindowTitle("File info");

    layout = new QVBoxLayout(q);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    topLayout = new QGridLayout();
    topLayout->setContentsMargins(5, 5, 5, 5);
    topLayout->setSpacing(5);

    iconLabel = new QLabel(q);
    iconLabel->setObjectName(QLatin1String("iconLabel"));

    nameLabel = new QLabel(q);
    nameLabel->setObjectName(QLatin1String("nameLabel"));
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    topLayout->addWidget(iconLabel, 0, 0, 1, 1);
    topLayout->addWidget(nameLabel, 0, 1, 1, 1);

    layout->addLayout(topLayout);

    QFrame *line = new QFrame(q);
    line->setObjectName(QLatin1String("line"));
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);

    widget = new OutlineWidget(q);
    layout->addWidget(widget);

    widget_1 = new QWidget;
    widget_1->setObjectName(QLatin1String("widget_1"));
    widget->addWidget(widget_1, q->tr("File Info"));

    layout_1 = new QFormLayout(widget_1);
    layout_1->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    layout_1->setSizeConstraint(QLayout::SetMinimumSize);

    sizeLabel = new QLabel(widget_1);
    sizeLabel->setObjectName(QLatin1String("sizeLabel"));

    size = new QLabel(widget_1);
    size->setObjectName(QLatin1String("size"));
    size->setTextInteractionFlags(Qt::TextBrowserInteraction);
    layout_1->addRow(sizeLabel, size);

    locationLabel = new QLabel(widget_1);
    locationLabel->setObjectName(QLatin1String("locationLabel"));

    location = new LabelLineEdit(widget_1);
    location->setObjectName(QLatin1String("location"));
    layout_1->addRow(locationLabel, location);

    createdLabel = new QLabel(widget_1);
    createdLabel->setObjectName(QLatin1String("createdLabel"));

    created = new QLabel(widget_1);
    created->setObjectName(QLatin1String("created"));
    created->setTextInteractionFlags(Qt::TextBrowserInteraction);
    layout_1->addRow(createdLabel, created);

    modifiedLabel = new QLabel(widget_1);
    modifiedLabel->setObjectName(QLatin1String("modifiedLabel"));

    modified = new QLabel(widget_1);
    modified->setObjectName(QLatin1String("modified"));
    modified->setTextInteractionFlags(Qt::TextBrowserInteraction);
    layout_1->addRow(modifiedLabel, modified);

    acceccedLabel = new QLabel(widget_1);
    acceccedLabel->setObjectName(QLatin1String("acceccedLabel"));

    accecced = new QLabel(widget_1);
    accecced->setObjectName(QLatin1String("accecced"));
    accecced->setTextInteractionFlags(Qt::TextBrowserInteraction);
    layout_1->addRow(acceccedLabel, accecced);

    widget_2 = new QWidget;
    widget_2->setObjectName(QLatin1String("widget_2"));
    widget->addWidget(widget_2, QObject::tr("Drive Info"));

    layout_2 = new QFormLayout(widget_2);
    layout_2->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    layout_2->setSizeConstraint(QLayout::SetMinimumSize);

    driveLabel = new QLabel(widget_2);
    driveLabel->setObjectName(QLatin1String("driveLabel"));

    drive = new QLabel(widget_2);
    drive->setObjectName(QLatin1String("drive"));
    drive->setTextInteractionFlags(Qt::TextBrowserInteraction);
    layout_2->addRow(driveLabel, drive);

    mountPointLabel = new QLabel(widget_2);
    mountPointLabel->setObjectName(QLatin1String("mountPointLabel"));

    mountPoint = new LabelLineEdit(widget_2);
    mountPoint->setObjectName(QLatin1String("mountPoint"));
    layout_2->addRow(mountPointLabel, mountPoint);

    fileSystemLabel = new QLabel(widget_2);
    fileSystemLabel->setObjectName(QLatin1String("fileSystemLabel"));

    fileSystem = new QLabel(widget_2);
    fileSystem->setObjectName(QLatin1String("fileSystem"));
    fileSystem->setTextInteractionFlags(Qt::TextBrowserInteraction);
    layout_2->addRow(fileSystemLabel, fileSystem);

    totalSizeLabel = new QLabel(widget_2);
    totalSizeLabel->setObjectName(QLatin1String("totalSizeLabel"));

    totalSize = new QLabel(widget_2);
    totalSize->setObjectName(QLatin1String("availableSize"));
    totalSize->setTextInteractionFlags(Qt::TextBrowserInteraction);
    layout_2->addRow(totalSizeLabel, totalSize);

    availableSizeLabel = new QLabel(widget_2);
    availableSizeLabel->setObjectName(QLatin1String("availableSizeLabel"));

    availableSize = new QLabel(widget_2);
    availableSize->setObjectName(QLatin1String("availableSize"));
    availableSize->setTextInteractionFlags(Qt::TextBrowserInteraction);
    layout_2->addRow(availableSizeLabel, availableSize);

    widget_3 = new QWidget;
    widget->addWidget(widget_3, QObject::tr("Permissions"));

    layout_3 = new QFormLayout(widget_3);
    layout_3->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    layout_3->setSizeConstraint(QLayout::SetMinimumSize);

    userPermissionsComboBox = new QComboBox(widget_3);
    userPermissionsComboBox->setObjectName(QLatin1String("userPermissionsComboBox"));
    userPermissionsComboBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout_3->addRow("User:", userPermissionsComboBox);

    groupPermissionsComboBox = new QComboBox(widget_3);
    groupPermissionsComboBox->setObjectName(QLatin1String("groupPermissionsComboBox"));
    groupPermissionsComboBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout_3->addRow("Group:", groupPermissionsComboBox);

    otherPermissionsComboBox = new QComboBox(widget_3);
    otherPermissionsComboBox->setObjectName(QLatin1String("otherPermissionsComboBox"));
    otherPermissionsComboBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout_3->addRow("Other:", otherPermissionsComboBox);
    layout_3->addRow(new QLabel, new QLabel);

    retranslateUi();
}

void FileInfoDialogPrivate::retranslateUi()
{
    sizeLabel->setText(QObject::tr("Size:"));
    locationLabel->setText(QObject::tr("Location:"));
    createdLabel->setText(QObject::tr("Created:"));
    modifiedLabel->setText(QObject::tr("Modified:"));
    acceccedLabel->setText(QObject::tr("Accessed:"));

    driveLabel->setText(QObject::tr("Drive:"));
    totalSizeLabel->setText(QObject::tr("Total size:"));
    availableSizeLabel->setText(QObject::tr("Available size:"));
    mountPointLabel->setText(QObject::tr("Mount point:"));
    fileSystemLabel->setText(QObject::tr("File system:"));

    widget->setText(0, QObject::tr("File Info"));
    widget->setText(1, QObject::tr("Drive Info"));
    widget->setText(2, QObject::tr("Permissions"));

    QStringList permissions;
    permissions << QObject::tr("Read only") << QObject::tr("Read write");

    userPermissionsComboBox->clear();
    userPermissionsComboBox->addItems(permissions);

    groupPermissionsComboBox->clear();
    groupPermissionsComboBox->addItems(permissions);

    otherPermissionsComboBox->clear();
    otherPermissionsComboBox->addItems(permissions);
}

FileInfoDialog::FileInfoDialog(QWidget *parent) :
    QWidget(parent),
    d(new FileInfoDialogPrivate(this))
{
    d->setupUi();
    d->directoryDetails = 0;

    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowFlags(Qt::Window);

    setMinimumSize(200, 400);
    setMaximumWidth(400);

    connect(d->userPermissionsComboBox, SIGNAL(activated(int)), SLOT(onActivatedUser(int)));
    connect(d->groupPermissionsComboBox, SIGNAL(activated(int)), SLOT(onActivatedGroup(int)));
    connect(d->otherPermissionsComboBox, SIGNAL(activated(int)), SLOT(onActivatedOther(int)));
}

FileInfoDialog::~FileInfoDialog()
{
    if (d->directoryDetails) {
        if (d->directoryDetails->isRunning())
            d->directoryDetails->stopProcessing();

        d->directoryDetails->wait();
    }

    delete d;
}

QFileInfo FileInfoDialog::fileInfo() const
{
    return d->fileInfo;
}

void FileInfoDialog::setFileInfo(const QFileInfo &info)
{
    d->fileInfo = info;
    d->driveInfo = QDriveInfo(d->fileInfo.absoluteFilePath());

    if (d->directoryDetails) {
        delete d->directoryDetails;
        d->directoryDetails = 0;
    }

    if (info.isDir()) {
        d->directoryDetails = new DirectoryDetails(info.absoluteFilePath(), this);
        connect(d->directoryDetails, SIGNAL(finished()), this, SLOT(updateSize()));
        d->directoryDetails->start();
    }

    d->updateUi();
}

QSize FileInfoDialog::minimumSizeHint() const
{
    return QSize(300, 400);
}

QSize FileInfoDialog::sizeHint() const
{
    this->ensurePolished();

    int w = 0, h = 0;
    for (int i = 0; i < d->widget->count(); i++) {
        QSize sizeHint = d->widget->widget(i)->sizeHint();
        w = qMax(sizeHint.width(), w);
        h += sizeHint.height() + 20;
    }
    return QSize(300, 100 + h);
}

void FileInfoDialog::onActivatedUser(int i)
{
    d->fileInfo.refresh();
    QFile::Permissions flags = (i == 1) ? (QFile::WriteOwner | QFile::ReadOwner) : (QFile::ReadOwner);
    QFile::setPermissions(d->fileInfo.filePath(), (d->fileInfo.permissions() & 0x1FF) | flags );
}

void FileInfoDialog::onActivatedGroup(int i)
{
    d->fileInfo.refresh();
    QFile::Permissions flags = (i == 1) ? (QFile::WriteGroup | QFile::ReadGroup) : (QFile::ReadGroup);
    QFile::setPermissions(d->fileInfo.filePath(), (d->fileInfo.permissions() & 0xFC7) | flags );
}

void FileInfoDialog::onActivatedOther(int i)
{
    d->fileInfo.refresh();
    QFile::Permissions flags = (i == 1) ? (QFile::WriteOther | QFile::ReadOther) : (QFile::ReadOther);
    QFile::setPermissions(d->fileInfo.filePath(), (d->fileInfo.permissions() & 0xFF8) | flags );
}

void FileInfoDialog::updateSize()
{
    int objCount = d->directoryDetails->totalFiles() + d->directoryDetails->totalFolders();
    qint64 totalSize = d->directoryDetails->totalSize();
    QString sizeLabel = tr("%2 for %3 objects").
            arg(sizeToString(totalSize)).
            arg(objCount);
    d->size->setText(sizeLabel);
}