#include <QHBoxLayout>

#include "statwidget.h"

CStatWidget::CStatWidget(const QString &title, int amount, QWidget *parent,
                         Qt::WindowFlags f) : QWidget(parent, f)
{
    QHBoxLayout *hbox = new QHBoxLayout(this);
    QLabel *l = new QLabel(title);
    l->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    hbox->addWidget(l);

    for (int i=0; i<amount; ++i)
    {
        l = new QLabel("1");
        l->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        l->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
        labelList << l;
        hbox->addWidget(l);
    }
}
