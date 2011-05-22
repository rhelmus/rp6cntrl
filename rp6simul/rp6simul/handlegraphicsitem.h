#ifndef HANDLEGRAPHICSITEM_H
#define HANDLEGRAPHICSITEM_H

#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>

class CHandleGraphicsItem : public QGraphicsRectItem
{
public:
    enum EHandlePos
    {
        HANDLE_NONE = 0x00,
        HANDLE_LEFT = 0x01,
        HANDLE_RIGHT = 0x02,
        HANDLE_TOP = 0x04,
        HANDLE_BOTTOM = 0x08,
        HANDLE_CENTER = 0x16
    };

    Q_DECLARE_FLAGS(EHandlePosFlags, EHandlePos)

private:
    EHandlePosFlags handlePos;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) { event->accept(); }
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) { event->accept(); }
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) { event->accept(); }

public:
    CHandleGraphicsItem(EHandlePosFlags pos, QGraphicsItem *parent = 0);

    EHandlePosFlags handlePosition(void) const { return handlePos; }
};

Q_DECLARE_OPERATORS_FOR_FLAGS(CHandleGraphicsItem::EHandlePosFlags)

#endif // HANDLEGRAPHICSITEM_H
