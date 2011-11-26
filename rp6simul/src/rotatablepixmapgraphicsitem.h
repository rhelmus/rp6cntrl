#ifndef ROTATABLEPIXMAPGRAPHICSITEM_H
#define ROTATABLEPIXMAPGRAPHICSITEM_H

#include "handlegraphicsitem.h"
#include "basegraphicsitem.h"

class CRotatablePixmapGraphicsItem : public CBaseGraphicsItem
{
    Q_OBJECT

    QRectF boundRect;
    QPixmap pixmap;
    typedef QMap<CHandleGraphicsItem::EHandlePosFlags, QGraphicsItem *> THandleList;
    THandleList handles;
    CHandleGraphicsItem *pressedHandle;
    mutable QPainterPath pmShape;
    mutable bool hasShape;

    void addHandle(CHandleGraphicsItem::EHandlePosFlags pos);
    void createHandles(void);
    void adjustHandles(void);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

public:
    CRotatablePixmapGraphicsItem(const QPixmap &pm, QGraphicsItem *parent = 0);
    CRotatablePixmapGraphicsItem(QGraphicsItem *parent = 0);

    QRectF boundingRect(void) const { return boundRect; }
    QPainterPath shape(void) const;

    void setPixmap(const QPixmap &pm);

signals:
    void rotationChanged(qreal); // Manual rotate
};

#endif // ROTATABLEPIXMAPGRAPHICSITEM_H
