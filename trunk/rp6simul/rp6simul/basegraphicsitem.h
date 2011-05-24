#ifndef BASEGRAPHICSITEM_H
#define BASEGRAPHICSITEM_H

#include <QGraphicsObject>

class QAction;
class QMenu;

class CBaseGraphicsItem : public QGraphicsObject
{
    Q_OBJECT

    bool isMovable;
    bool dragging;    
    QPointF mouseDragPos, oldPos;

    void updateMouseCursor(bool selected);
    void alignToGrid(QPointF pos);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

public:
    CBaseGraphicsItem(QGraphicsItem *parent = 0);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *);

    void setMovable(bool m) { isMovable = m; updateMouseCursor(m); }

signals:
    void posChanged(void);
};

#endif // BASEGRAPHICSITEM_H
