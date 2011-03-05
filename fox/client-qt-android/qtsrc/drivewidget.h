#ifndef DRIVEWIDGET_H
#define DRIVEWIDGET_H

#include <QFrame>
#include <QVector2D>

class QLabel;
class QMouseEvent;

class CDriveWidget: public QFrame
{
    QVector2D startDragPos, endDragPos;

protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

public:
    CDriveWidget(QWidget *parent = 0);
};

#endif // DRIVEWIDGET_H
