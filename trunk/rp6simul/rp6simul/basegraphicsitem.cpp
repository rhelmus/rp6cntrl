#include "basegraphicsitem.h"
#include "robotscene.h"

#include <QtGui>

CBaseGraphicsItem::CBaseGraphicsItem(QGraphicsItem *parent)
    : QGraphicsObject(parent), isMovable(true), snapsToGrid(true),
      isDeletable(true), dragging(false)
{
    setFlag(ItemIsSelectable);
    setFlag(ItemIsFocusable);
    setAcceptedMouseButtons(Qt::LeftButton);
    updateMouseCursor(false);

    connect(&dragTimer, SIGNAL(timeout()), this, SLOT(updateDrag()));
    dragTimer.setInterval(10);
    dragTimer.setSingleShot(true);
}

void CBaseGraphicsItem::updateMouseCursor(bool selected)
{
    if (!selected || !isMovable)
        unsetCursor();
    else
        setCursor(Qt::SizeAllCursor);
}

void CBaseGraphicsItem::removeMe()
{
    emit removed(this);
    deleteLater();
}

void CBaseGraphicsItem::updateDrag()
{
    QRectF r(boundingRect());
    const QPointF offset(startDragPos - r.center());
    r.moveCenter(mapToParent(curDragPos - offset));

    QPointF newp;
    if (snapsToGrid)
    {
        CRobotScene *rscene = qobject_cast<CRobotScene *>(scene());
        Q_ASSERT(rscene);
        if (rscene->getAutoGrid())
            newp = rscene->alignPosToGrid(r.topLeft());
    }

    if (newp.isNull())
        newp = r.topLeft();

    if (newp != pos())
    {
        setPos(newp);
        emit posChanged(oldPos);
        oldPos = newp;
    }
    /*
    if (oldPos != pos())
    {
        emit posChanged(oldPos);
        oldPos = pos();
    }*/
}

void CBaseGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (isMovable)
    {
        startDragPos = event->pos();
        oldPos = pos();
        dragging = true;
    }

    QGraphicsItem::mousePressEvent(event);
}

void CBaseGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (dragging)
    {
#if 1
        QRectF r(boundingRect());
        const QPointF offset(startDragPos - r.center());
        r.moveCenter(mapToParent(event->pos() - offset));

        QPointF newp;
        if (snapsToGrid)
        {
            CRobotScene *rscene = qobject_cast<CRobotScene *>(scene());
            Q_ASSERT(rscene);
            if (rscene->getAutoGrid())
                newp = rscene->alignPosToGrid(r.topLeft());
        }

        if (newp.isNull())
            newp = r.topLeft();

        if (newp != pos())
        {
            setPos(newp);
            emit posChanged(oldPos);
            oldPos = newp;
        }

#elif 0
        curDragPos = event->pos();
        if (!dragTimer.isActive())
            dragTimer.start();
#else
        QRectF r(boundingRect());
        const QPointF offset(startDragPos - r.center());
        r.moveCenter(mapToParent(event->pos() - offset));

        QPointF newp;
        if (snapsToGrid)
        {
            CRobotScene *rscene = qobject_cast<CRobotScene *>(scene());
            Q_ASSERT(rscene);
            if (rscene->getAutoGrid())
                newp = rscene->alignPosToGrid(r.topLeft());
        }

        if (newp.isNull())
            newp = r.topLeft();

        if (newp != pos())
        {
            setPos(newp);
            curDragPos = newp;
            if (!dragTimer.isActive())
                dragTimer.start();
        }
#endif
    }

    QGraphicsItem::mouseMoveEvent(event);
}

void CBaseGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (isMovable && dragging)
        dragging = false;

    QGraphicsItem::mouseReleaseEvent(event);
}

void CBaseGraphicsItem::keyReleaseEvent(QKeyEvent *event)
{
    if (isDeletable && (event->key() == Qt::Key_Delete))
    {
        removeMe();
        event->accept();
    }
    else
        event->ignore();

    QGraphicsItem::keyReleaseEvent(event);
}

void CBaseGraphicsItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    QMenu menu;
    QAction *aligna = 0, *dela = 0;

    if (isMovable)
        aligna = menu.addAction(QIcon("../resource/grid-icon.png"),
                                "Align to grid");

    if (isDeletable)
    {
        dela = menu.addAction(QIcon("../resource/delete.png"), "Delete");
        // NOTE: Although the shortcut is set here, real key presses are
        // handled in keyReleaseEvent()
        dela->setShortcut(Qt::Key_Delete);
    }

    if (menu.isEmpty())
        return;

    QAction *a = menu.exec(event->screenPos());

    if (a) // Need to check as variables may 0 as well.
    {
        if (a == aligna)
        {
            CRobotScene *rscene = qobject_cast<CRobotScene *>(scene());
            Q_ASSERT(rscene);
            const QPointF opos(pos());
            const QPointF npos(rscene->alignPosToGrid(pos()));
            if (opos != npos)
            {
                setPos(npos);
                emit posChanged(opos);
            }
        }
        else if (a == dela)
            removeMe();
    }

    QGraphicsItem::contextMenuEvent(event);
}

QVariant CBaseGraphicsItem::itemChange(GraphicsItemChange change,
                                       const QVariant &value)
{
    if (change == ItemSelectedHasChanged)
    {
        const bool s = value.toBool();
        updateMouseCursor(s);
        setZValue(s ? 1.0 : 0.0);
    }

    return QGraphicsItem::itemChange(change, value);
}

void CBaseGraphicsItem::paint(QPainter *painter,
                              const QStyleOptionGraphicsItem *option, QWidget *)
{
    if (option->state & QStyle::State_Selected)
    {
        painter->save();
        painter->setPen(QPen(Qt::black, 0.7, Qt::DashLine));
        painter->drawRect(boundingRect());
        painter->restore();
    }
}
