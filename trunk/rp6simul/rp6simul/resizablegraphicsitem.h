#ifndef RESIZABLEGRAPHICSITEM_H
#define RESIZABLEGRAPHICSITEM_H

// Code inspired by http://www.qtcentre.org/threads/18238-Drawing-and-resizing-shapes

#include "handlegraphicsitem.h"

#include <QGraphicsItem>

class CResizableGraphicsItem : public QGraphicsItem
{
    QGraphicsItem *containingItem;
    typedef QHash<CHandleGraphicsItem::EHandlePosFlags, QGraphicsItem *> THandleList;
    THandleList handles;
    CHandleGraphicsItem *pressedHandle;
    QPointF lastMousePos;

    void addHandle(CHandleGraphicsItem::EHandlePosFlags pos);
    void adjustHandles(void);
    void updateGeometry(const QPointF &mousepos);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) { }
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

public:
    CResizableGraphicsItem(QGraphicsItem *citem, QGraphicsItem *parent = 0);

    QRectF boundingRect(void) const { return containingItem->boundingRect(); }
    QPainterPath shape(void) const { return containingItem->shape(); }

    void setSize(qreal w, qreal h);
    void setSize(const QSizeF &s) { setSize(s.width(), s.height()); }
};

#endif // RESIZABLEGRAPHICSITEM_H
