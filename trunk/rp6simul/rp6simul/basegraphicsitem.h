#ifndef BASEGRAPHICSITEM_H
#define BASEGRAPHICSITEM_H

#include <QGraphicsObject>
#include <QTimer>

class QAction;
class QMenu;

class CBaseGraphicsItem : public QGraphicsObject
{
    Q_OBJECT

    bool isMovable, snapsToGrid;
    bool isDeletable;
    bool dragging;
    QPointF startDragPos, oldPos;

    void updateMouseCursor(bool selected);
    void removeMe(void);

    virtual qreal getZValue(void) const { return 0.0; }

protected:
    bool getSnapsToGrid(void) const { return snapsToGrid; }

    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

public:
    CBaseGraphicsItem(QGraphicsItem *parent = 0);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *);

    void setMovable(bool m) { isMovable = m; updateMouseCursor(m); }
    void setSnapsToGrid(bool s) { snapsToGrid = s; }
    void setDeletable(bool d) { isDeletable = d; }

signals:
    void posChanged(const QPointF &);
    void removed(CBaseGraphicsItem *);
};

#endif // BASEGRAPHICSITEM_H