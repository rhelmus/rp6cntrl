#ifndef RESIZABLEPIXMAPGRAPHICSITEM_H
#define RESIZABLEPIXMAPGRAPHICSITEM_H

#include "handlegraphicsitem.h"

#include "basegraphicsitem.h"

class CResizablePixmapGraphicsItem : public CBaseGraphicsItem
{
    Q_OBJECT

    QRectF boundRect;
    QPixmap pixmap;
    bool tiled;
    bool isResizable;
    typedef QHash<CHandleGraphicsItem::EHandlePosFlags, QGraphicsItem *> THandleList;
    THandleList handles;
    CHandleGraphicsItem *pressedHandle;
    QPolygonF oldBoundRect;
    mutable QPainterPath pmShape;
    mutable bool hasShape;

    void addHandle(CHandleGraphicsItem::EHandlePosFlags pos);
    void adjustHandles(void);
    void updateGeometry(const QPointF &mousepos);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

public:
    CResizablePixmapGraphicsItem(const QPixmap &pm, bool t,
                                 QGraphicsItem *parent = 0);

    QRectF boundingRect(void) const { return boundRect; }
    QPainterPath shape(void) const;

    void setSize(const QSizeF &s);
    void setSize(qreal w, qreal h) { setSize(QSizeF(w, h)); }
    void setResizable(bool r);
    QSizeF getSize(void) const { return boundRect.size(); }

signals:
    void sizeChanged(void);
};

#endif // RESIZABLEPIXMAPGRAPHICSITEM_H
