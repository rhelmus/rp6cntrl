#ifndef RESIZABLEGRAPHICSITEM_H
#define RESIZABLEGRAPHICSITEM_H

// Code inspired by http://www.qtcentre.org/threads/18238-Drawing-and-resizing-shapes

#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>

// UNDONE
#include <QPainter>

class CResizableGraphicsItem : public QGraphicsItem
{
    enum
    {
        HANDLE_NONE = 0x00,
        HANDLE_LEFT = 0x01,
        HANDLE_RIGHT = 0x02,
        HANDLE_TOP = 0x04,
        HANDLE_BOTTOM = 0x08
    };

    QGraphicsItem *containingItem;
    typedef QHash<int, QGraphicsItem *> THandleList;
    THandleList handles;
    int pressedHandlePos;

    void addHandle(int pos);
    void adjustHandles(void);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) { }

public:
    CResizableGraphicsItem(QGraphicsItem *citem, QGraphicsItem *parent = 0);

    QRectF boundingRect(void) const { return containingItem->boundingRect(); }
//    QPainterPath shape(void) const { return containingItem->shape(); }
};

#endif // RESIZABLEGRAPHICSITEM_H
