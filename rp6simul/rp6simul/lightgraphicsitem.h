#ifndef LIGHTGRAPHICSITEM_H
#define LIGHTGRAPHICSITEM_H

#include <QGraphicsItem>

class CHandleGraphicsItem;

class CLightGraphicsItem : public QGraphicsItem
{
    float radius;
    bool dragging, handleDragging;
    QPointF dragMousePos, radiusDragMousePos;
    CHandleGraphicsItem *radiusHandle;

    void setRadiusHandlePos(void);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

public:
    CLightGraphicsItem(float r, QGraphicsItem *parent = 0);

    QRectF boundingRect(void) const
    { return QRectF(0.0, 0.0, radius*2.0, radius*2.0); }
    QPainterPath shape(void) const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *,
               QWidget *);

    float getRadius(void) const { return radius; }
    float intensityAt(const QPointF &p,
                      const QList<QPolygonF> &obstacles) const;
};

#endif // LIGHTGRAPHICSITEM_H
