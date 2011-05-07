#ifndef ROBOTGRAPHICSITEM_H
#define ROBOTGRAPHICSITEM_H

#include <QGraphicsPixmapItem>

class CRobotGraphicsItem : public QGraphicsPixmapItem
{
    int leftPower, rightPower;
    int skipFrames;

    QPointF mapDeltaPos(qreal x, qreal y) const;
    bool tryMove(float lpower, float rpower);
    QList<QGraphicsItem *> tryDoMove(float rotspeed, QPointF dpos);

protected:
    void advance(int phase);

public:
    CRobotGraphicsItem(void);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);

    void setLeftMotor(int power) { leftPower = power; }
    void setRightMotor(int power) { rightPower = power; }
};

#endif // ROBOTGRAPHICSITEM_H
