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

    typedef QHash<int, QGraphicsItem *> THandleList;
    THandleList handles;
    int pressedHandlePos;
    QRectF boundRect;

    void addHandle(int pos);
    void adjustHandles(void);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

    // UNDONE
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        painter->drawRect(boundingRect());
    }

public:
    CResizableGraphicsItem(QGraphicsItem *parent = 0);

    QRectF boundingRect(void) const { return boundRect; }
};

#endif // RESIZABLEGRAPHICSITEM_H
