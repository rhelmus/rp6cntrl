#include <QtGui>
#include "pathinput.h"

CPathInput::CPathInput(const QString &desc, EPathInputMode mode,
                       const QString &path, const QString &f, QWidget *parent)
    : QWidget(parent), pathDescription(desc), filter(f), pathInputMode(mode)
{
    QHBoxLayout *hbox = new QHBoxLayout(this);

    hbox->addWidget(pathEdit = new QLineEdit);
    if (!path.isEmpty())
        pathEdit->setText(path);
    connect(pathEdit, SIGNAL(textChanged(const QString &)), this,
            SIGNAL(pathTextChanged(const QString &)));
    connect(pathEdit, SIGNAL(textEdited(const QString &)), this,
            SIGNAL(pathTextEdited(const QString &)));

    QCompleter *comp = new QCompleter(pathEdit);
    QDirModel *dm = new QDirModel(comp);
    if ((mode == PATH_EXISTDIR) || (mode == PATH_NEWFILE))
        dm->setFilter(dm->filter() & ~QDir::Files);
    comp->setModel(dm);
    pathEdit->setCompleter(comp);

    QToolButton *openButton = new QToolButton;
    openButton->setIcon(QIcon(style()->standardIcon(QStyle::SP_DirOpenIcon)));
    connect(openButton, SIGNAL(clicked()), this, SLOT(openBrowser()));
    hbox->addWidget(openButton);
}

void CPathInput::openBrowser()
{
    QString path;
    if (pathInputMode == PATH_EXISTDIR)
        path = QFileDialog::getExistingDirectory(this, pathDescription,
                                                 pathEdit->text());
    else if (pathInputMode == PATH_EXISTFILE)
        path = QFileDialog::getOpenFileName(this, pathDescription,
                                            pathEdit->text(), filter);
    else if (pathInputMode == PATH_NEWFILE)
        path = QFileDialog::getSaveFileName(this, pathDescription,
                                            pathEdit->text(), filter);

    if (!path.isEmpty())
        pathEdit->setText(path);
}

QString CPathInput::getPath() const
{
    return pathEdit->text();
}

void CPathInput::setPath(const QString &path)
{
    pathEdit->setText(path);
}
