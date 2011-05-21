#ifndef LIGHTGRAPHICSITEM_H
#define LIGHTGRAPHICSITEM_H

#include <QGraphicsItem>

class CLightGraphicsItem : public QGraphicsItem
{
    float radius;

public:
    CLightGraphicsItem(float r, QGraphicsItem *parent = 0);

    QRectF boundingRect(void) const
    { return QRectF(0.0, 0.0, radius*2.0, radius*2.0); }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *,
               QWidget *);

    float intensityAt(const QPointF &p,
                      const QList<QPolygonF> &obstacles) const;
};

#endif // LIGHTGRAPHICSITEM_H
