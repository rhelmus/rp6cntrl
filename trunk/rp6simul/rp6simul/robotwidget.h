#ifndef ROBOTWIDGET_H
#define ROBOTWIDGET_H

#include <QWidget>

class CRobotWidget : public QWidget
{
    Q_OBJECT

    const QSize robotImgSize;
    const QSize widgetExtraSize;
    QPixmap robotPixmap;

protected:
    void paintEvent(QPaintEvent *);

public:
    explicit CRobotWidget(QWidget *parent = 0);

    QSize minimumSizeHint(void) const { return robotImgSize + widgetExtraSize; }
};

#endif // ROBOTWIDGET_H
