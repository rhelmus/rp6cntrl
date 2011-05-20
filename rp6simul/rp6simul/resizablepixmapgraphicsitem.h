#ifndef RESIZABLEPIXMAPGRAPHICSITEM_H
#define RESIZABLEPIXMAPGRAPHICSITEM_H

#include "handlegraphicsitem.h"

#include <QGraphicsItem>

class CResizablePixmapGraphicsItem : public QGraphicsItem
{
    QRectF boundRect;
    QPixmap pixmap;
    bool tiled;
    bool isMovable, isResizable;
    typedef QHash<CHandleGraphicsItem::EHandlePosFlags, QGraphicsItem *> THandleList;
    THandleList handles;
    CHandleGraphicsItem *pressedHandle;
    QPointF lastMousePos;
    bool dragging;
    mutable QPainterPath pmShape;
    mutable bool hasShape;

    void updateMouseCursor(bool selected);
    void addHandle(CHandleGraphicsItem::EHandlePosFlags pos);
    void adjustHandles(void);
    void updateGeometry(const QPointF &mousepos);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *,
               QWidget *);
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

public:
    CResizablePixmapGraphicsItem(const QPixmap &pm, bool t,
                                 QGraphicsItem *parent = 0);

    QRectF boundingRect(void) const { return boundRect; }
    QPainterPath shape(void) const;

    void setSize(const QSizeF &s);
    void setSize(qreal w, qreal h) { setSize(QSizeF(w, h)); }
    void setMovable(bool m);
    void setResizable(bool r);
};

#endif // RESIZABLEPIXMAPGRAPHICSITEM_H
