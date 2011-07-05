#ifndef LIGHTGRAPHICSITEM_H
#define LIGHTGRAPHICSITEM_H

#include "basegraphicsitem.h"
#include "graphicsitemtypes.h"

class CHandleGraphicsItem;

class CLightGraphicsItem : public CBaseGraphicsItem
{
    Q_OBJECT

    float radius, oldRadius;
    bool handleDragging;
    QPointF radiusDragMousePos;
    CHandleGraphicsItem *radiusHandle;

    void setRadiusHandlePos(void);

protected:
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

public:
    CLightGraphicsItem(float r, QGraphicsItem *parent = 0);

    QRectF boundingRect(void) const
    { return QRectF(0.0, 0.0, radius*2.0, radius*2.0); }
    QPainterPath shape(void) const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);

    float getRadius(void) const { return radius; }

    enum { Type = GRAPHICSITEM_TYPELIGHT };
    int type(void) const { return Type; }

signals:
    void radiusChanged(float);
};

#endif // LIGHTGRAPHICSITEM_H
