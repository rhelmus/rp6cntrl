#include "progressdialog.h"

#include <QtGui>

CProgressDialog::CProgressDialog(bool c, QWidget *parent) :
    QDialog(parent), closable(c), canceled(false)
{
    QVBoxLayout *vbox = new QVBoxLayout(this);

    setModal(true);

    vbox->addWidget(progressBar = new QProgressBar);
    vbox->addWidget(statusText = new QLabel);

    if (closable)
    {
        QDialogButtonBox *bbox = new QDialogButtonBox(QDialogButtonBox::Cancel);
        connect(bbox, SIGNAL(rejected()), this, SLOT(cancelPressed()));
        vbox->addWidget(bbox);
    }
}

void CProgressDialog::closeEvent(QCloseEvent *e)
{
    qDebug() << "closable:" << closable;
    canceled = true;
    if (closable)
        QDialog::closeEvent(e);
    else
        e->ignore();
}

void CProgressDialog::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape)
    {
        canceled = true;
        if (!closable)
        {
            e->ignore();
            return;
        }
    }

    QDialog::keyPressEvent(e);
}

void CProgressDialog::setValue(int v)
{
    progressBar->setValue(v);
}

void CProgressDialog::setRange(int min, int max)
{
    progressBar->setRange(min, max);
}

void CProgressDialog::setLabelText(const QString &t)
{
    statusText->setText(t);
}
