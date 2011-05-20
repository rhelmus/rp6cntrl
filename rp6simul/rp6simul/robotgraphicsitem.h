#ifndef ROBOTGRAPHICSITEM_H
#define ROBOTGRAPHICSITEM_H

#include "handlegraphicsitem.h"

#include <QGraphicsPixmapItem>

class CRobotGraphicsItem : public QGraphicsPixmapItem
{
    int leftPower, rightPower;
    int skipFrames;

    typedef QMap<CHandleGraphicsItem::EHandlePosFlags, QGraphicsItem *> THandleList;
    THandleList handles;
    QPointF lastMousePos;
    bool dragging;
    CHandleGraphicsItem *pressedHandle;

    void updateMouseCursor(bool selected);
    void addHandle(CHandleGraphicsItem::EHandlePosFlags pos);
    QPointF mapDeltaPos(qreal x, qreal y) const;
    QPointF mapDeltaPos(const QPointF &p) const { return mapDeltaPos(p.x(), p.y()); }
    bool tryMove(float lpower, float rpower);
    QList<QGraphicsItem *> tryDoMove(float rotspeed, QPointF dpos);

protected:
    void advance(int phase);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

public:
    CRobotGraphicsItem(void);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);

    void setLeftMotor(int power) { leftPower = power; }
    void setRightMotor(int power) { rightPower = power; }
};

#endif // ROBOTGRAPHICSITEM_H
