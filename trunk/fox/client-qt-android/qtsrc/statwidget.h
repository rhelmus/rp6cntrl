#ifndef STATWIDGET_H
#define STATWIDGET_H

#include <QLabel>

class CStatWidget: public QWidget
{
    QList<QLabel *> labelList;

public:
    CStatWidget(const QString &title, int amount,
                QWidget *parent = 0, Qt::WindowFlags f = 0);

    void setValue(int index, int value)
    { setValue(index, (QString()).setNum(value)); }
    void setValue(int index, const QString &value)
    { labelList.at(index)->setText(value); }
};

#endif // STATWIDGET_H
