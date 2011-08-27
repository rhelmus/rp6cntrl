#include "bumper.h"
#include "handlegraphicsitem.h"
#include "irsensor.h"
#include "led.h"
#include "lightgraphicsitem.h"
#include "robotgraphicsitem.h"
#include "simulator.h"
#include "utils.h"

#include <math.h>

#include <QtGui>

namespace {

qreal toClockwiseAngle(qreal a)
{
    // Convert from counter-clockwise + 90 to clockwise + 0
    return 90.0 - a;
}

qreal toCounterClockwiseAngle(qreal a)
{
    // Convert from clockwise + 0 to counter-clockwise + 90
    // (yes this is the same as the other way around)
    return 90.0 - a;
}

// Check if one of the obstacles actually causes a collision
bool checkCollidingItems(const QList<QGraphicsItem *> &obstacles,
                         QGraphicsItem *parent)
{
    foreach (QGraphicsItem *it, obstacles)
    {
        if (parent &&
            ((it->parentItem() == parent) || (it == parent)))
            continue;

        if (!qgraphicsitem_cast<CLightGraphicsItem *>(it) &&
            !qgraphicsitem_cast<CHandleGraphicsItem *>(it))
            return true;
    }

    return false;
}

float getRobotFrameSpeed(int motorspeed, float frametime,
                         EMotorDirection dir, float cmperpx)
{
    if (!motorspeed)
        return 0.0;

    // UNDONE: Make this configurable/load from plugin
    const float encresolution = 0.24;
    const float speedtimerbase = 200.0;

    // Necessary correction for RP6 library bug: see motor driver
    const float effspeedtimerbase = 1.1 * (speedtimerbase + 2.0);

    const float speedfreq = 1000.0 / effspeedtimerbase;
    const float counts_s = (float)motorspeed * speedfreq;
    const float mm_s = counts_s * encresolution;
    const float px_s = mm_s / (cmperpx * 10.0);

    // Amount of pixels moved during this frame
    float ret = px_s / 1000.0 * frametime;

    if (dir == MOTORDIR_BWD)
        ret = -ret;

    return ret;
}

}

CRobotGraphicsItem::CRobotGraphicsItem(QGraphicsItem *parent)
    : CResizablePixmapGraphicsItem(parent), skipFrames(0), pressedHandle(0)
{
    QPixmap pm("../resource/rp6-top.png");
    origRobotSize = pm.size();
    setPixmap(pm.scaledToWidth(120, Qt::SmoothTransformation), false);

    setTransformOriginPoint(boundingRect().center());
    setResizable(false);
    setDeletable(false);
    setSnapsToGrid(false);

    addHandle(CHandleGraphicsItem::HANDLE_LEFT |
              CHandleGraphicsItem::HANDLE_TOP);
    addHandle(CHandleGraphicsItem::HANDLE_LEFT |
              CHandleGraphicsItem::HANDLE_BOTTOM);
    addHandle(CHandleGraphicsItem::HANDLE_RIGHT |
              CHandleGraphicsItem::HANDLE_TOP);
    addHandle(CHandleGraphicsItem::HANDLE_RIGHT |
              CHandleGraphicsItem::HANDLE_BOTTOM);

    connect(this, SIGNAL(posChanged(const QPointF &)), SLOT(updateBumpers()));

    IRSensorUpdateTimer = new QTimer(this);
    IRSensorUpdateTimer->setInterval(100);
    connect(IRSensorUpdateTimer, SIGNAL(timeout()), SLOT(updateIRSensors()));
}

void CRobotGraphicsItem::addHandle(CHandleGraphicsItem::EHandlePosFlags pos)
{
    QGraphicsRectItem *handle = new CHandleGraphicsItem(pos, this);
    handle->hide();
    handle->setCursor(QPixmap("../resource/rotate.png"));

    QRectF hrect = handle->boundingRect();
    const QRectF myrect(boundingRect());

    hrect.moveCenter(myrect.center());

    if (pos & CHandleGraphicsItem::HANDLE_LEFT)
        hrect.moveLeft(myrect.left());
    else if (pos & CHandleGraphicsItem::HANDLE_RIGHT)
        hrect.moveRight(myrect.right());
    if (pos & CHandleGraphicsItem::HANDLE_TOP)
        hrect.moveTop(myrect.top());
    else if (pos & CHandleGraphicsItem::HANDLE_BOTTOM)
        hrect.moveBottom(myrect.bottom());

    handle->setPos(hrect.topLeft());

    handles[pos] = handle;
}

QPointF CRobotGraphicsItem::mapDeltaPos(qreal x, qreal y) const
{
    return (mapToParent(x, y) - mapToParent(0.0, 0.0));
}

qreal CRobotGraphicsItem::getPixmapScale() const
{
    // Equal aspect ratio, so scaling equals for width and height
    return boundingRect().width() / (qreal)origRobotSize.width();
}

bool CRobotGraphicsItem::handleObstacles(bool checkbumpers)
{
    const bool ret = checkCollidingItems(collidingItems(), this);

    if (checkbumpers)
        updateBumpers();

    return ret;
}

void CRobotGraphicsItem::tryMove()
{
    /*
      This function tries to move the robot, depending on its current
      motor speeds. For the movement two variables are used: movespeed
      and degspeed. The first is used to move the robot forwards/backwards,
      while the second controls how much the robot rotates.

      The movespeed holds the amount of pixels that should be moved during
      one frame (the delay between a frame is retrieved by
      CRobotScene::getRobotAdvanceDelay()). For this the counts/s are
      calculated from the motor speeds, converted to real distance (mm/s) and
      finally to pixels/s (px/s). The conversion to real distance happens
      in a similar way the DIST_MM macro from the RP6 library, using the encoder
      resolution. For mm/s --> px/s conversion the cmPerPixel property (see
      properties.lua) is used.

      The degspeed holds the amount of degrees that should be rotated during
      this frame. It is calculated in a similar way as the movespeed. To
      convert from pixels to degrees, it is assumed that the radius is equal to
      the length of the robot. This also explains the default value of
      'ROTATION_FACTOR' defined in the RP6 library. This value is also based
      on this length and holds the amount of counts per degree * 100.
    */

    const CRobotScene *rscene = qobject_cast<CRobotScene *>(scene());
    Q_ASSERT(rscene);
    // Delay between calls to this function (frame time)
    const float advdelay = rscene->getRobotAdvanceDelay();

    // Correct real distance calculations for different image scale
    const qreal scale = getPixmapScale();
    const float cmperpx =
            CSimulator::getInstance()->getRobotProperty("scale", "cmPerPixel").toFloat() / scale;

    const float lspeed =
            getRobotFrameSpeed(motorSpeed[MOTOR_LEFT], advdelay,
                               motorDirection[MOTOR_LEFT], cmperpx);
    const float rspeed =
            getRobotFrameSpeed(motorSpeed[MOTOR_RIGHT], advdelay,
                               motorDirection[MOTOR_RIGHT], cmperpx);

    // The final move speed is simply the average of both motor speeds.
    const float movespeed = (lspeed + rspeed) / 2.0;

    /*
      Both the move() and rotate() functions from the RP6 library try to
      improve their accuracy by slowing down before the target encoder
      counts (ie. target distance) is reached. However this only seems to have
      a significant effect on move(), because this function sets it
      'pre-deaccelerate' variable depending on the current driving speed (ie.
      slow down earlier at high speeds). The rotate() function always slows down
      100 encoder counts before reaching its target. This value is much too low
      at high speeds whith the relative low speed update frequency (~5 Hz) used.
      Therefore a positive bias (ie. turn too far) can clearly be seen when
      rotation is simulated (without any artificial deviations).
    */
    // UNDONE: Correct for modified ROTATION_FACTOR values
    // UNDONE: Somehow correct for positive bias caused by low speed update
    //         frequency of RP6 library code?
    const float rotspeed = (lspeed - rspeed) / 2.0;
    float degspeed = 0.0;
    if (rotspeed != 0.0)
    {
        const float robotlengthmm = 10.0 *
                CSimulator::getInstance()->getRobotProperty("robotLength", "length").toFloat();
        const float robotlengthpx = robotlengthmm / (cmperpx * 10.0);
        const float perimeter = robotlengthpx * M_PI;
        const float pxperdeg = perimeter / 360.0;
        degspeed = rotspeed / pxperdeg;
    }

    /*
      Try to move with calculated move- and rotation speeds. This may fail if
      the robot collides with any obstacles after having rotated/moved. In
      the case of failure try to unstuck the robot by moving it in
      random close positions.
    */
    if (!tryDoMove(degspeed, mapDeltaPos(0.0, -movespeed), true))
    {
        skipFrames = 3;

        const float l = 0.25, h = 0.75, inc = 0.25;
        for (float dx = l; dx <= h; dx += inc)
        {
            if (tryDoMove(0.0, mapDeltaPos(dx, 0.0), false))
                return;

            if (tryDoMove(0.0, mapDeltaPos(-dx, 0.0), false))
                return;

            for (float dy = l; dy <= h; dy += inc)
            {
                if (tryDoMove(0.0, mapDeltaPos(0.0, dy), false))
                    return;
                if (tryDoMove(0.0, mapDeltaPos(0.0, -dy), false))
                    return;
                if (tryDoMove(0.0, mapDeltaPos(dx, dy), false))
                    return;
                if (tryDoMove(0.0, mapDeltaPos(-dx, dy), false))
                    return;
            }
        }
    }

    skipFrames = 0;
}

bool CRobotGraphicsItem::tryDoMove(float rotspeed, QPointF dpos,
                                   bool checkbumpers)
{
    const QPointF oldpos(pos());
    const qreal oldrot = rotation();

    setRotation(rotation() + rotspeed);
    if (handleObstacles(checkbumpers))
    {
        setRotation(oldrot);
        return false;
    }
    else
    {
        setPos(pos() + dpos);
        if (handleObstacles(checkbumpers))
        {
            setPos(oldpos);
            return false;
        }
    }

    return true;
}

void CRobotGraphicsItem::updateBumpers()
{
    foreach (CBumper *b, bumpers)
    {
        const bool hit =
                checkCollidingItems(bumperItems[b]->collidingItems(), this);
        if (hit != b->isHit())
        {
            bumperItems[b]->setVisible(hit);
            b->setHit(hit);
            emit bumperChanged(b, hit);
        }
    }
}

void CRobotGraphicsItem::updateIRSensors()
{
    foreach (CIRSensor *ir, IRSensors)
    {
        // Start with line which has a zero degree angle
        // (in Qt's counter clockwise system)
        QPointF start(ir->getPosition());
        const qreal scale = getPixmapScale();
        start.rx() *= scale;
        start.ry() *= scale;

        QLineF line(start, start + QPointF(ir->getTraceDistance() * scale, 0.0));

        line.setAngle(toCounterClockwiseAngle(ir->getTraceAngle()));

        QPolygonF poly = mapToScene(line.x1(), line.y1(), line.x2(), line.y2());
        QList<QGraphicsItem *> obstacles = scene()->items(poly);

        foreach (QGraphicsItem *ob, obstacles)
        {
            if ((ob == this) || (ob->parentItem() == this))
                continue;
            QPolygonF obpoly(mapFromItem(ob, ob->boundingRect()));
            for (QPolygonF::iterator it=obpoly.begin(); it!=(obpoly.end()-1); ++it)
            {
                QLineF obl(*it, *(it+1));
                QPointF interp;
                if (line.intersect(obl, &interp) == QLineF::BoundedIntersection)
                {
                    QLineF tmp(line.p1(), interp);
                    if (tmp.length() < line.length())
                        line.setP2(interp);
                }
            }
        }

        const float oldd = ir->getHitDistance();
        const float newd = line.length() / scale;
        ir->setHitDistance(newd);
        if (oldd != newd)
            emit IRSensorChanged(ir, newd);
    }
}

void CRobotGraphicsItem::advance(int phase)
{
    if (!phase)
        return;

    if (skipFrames)
    {
        skipFrames--;
        return;
    }

    if (motorSpeed[MOTOR_LEFT] || motorSpeed[MOTOR_RIGHT])
    {
        const QPointF prepos(pos());
        const qreal oldrot = rotation();
        tryMove();
        if ((pos() != prepos) || (oldrot != rotation()))
            emit robotMoved(prepos, oldrot);
    }
}

bool CRobotGraphicsItem::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
    if (event->type() == QEvent::GraphicsSceneMousePress)
    {
        pressedHandle = dynamic_cast<CHandleGraphicsItem *>(watched);
        Q_ASSERT(pressedHandle);
    }
    else if (event->type() == QEvent::GraphicsSceneMouseRelease)
        pressedHandle = 0;
    else if (event->type() == QEvent::GraphicsSceneMouseMove)
    {
        QGraphicsSceneMouseEvent *me =
                static_cast<QGraphicsSceneMouseEvent*>(event);

        const QPointF start(mapToScene(boundingRect().center()));
        const QPointF end(mapToScene(mapFromItem(pressedHandle, me->pos())));
        const qreal mangle = toClockwiseAngle(QLineF(start, end).angle());

        if (mangle != rotation())
        {
            const qreal olda = rotation();
            setRotation(mangle);
            emit rotationChanged(olda);
        }
    }

    return false; // Propegate
}

QVariant CRobotGraphicsItem::itemChange(GraphicsItemChange change,
                                        const QVariant &value)
{
    if (change == ItemSceneHasChanged)
    {
        foreach(QGraphicsItem *h, handles)
            h->installSceneEventFilter(this);
    }
    else if (change == ItemSelectedHasChanged)
    {
        foreach(QGraphicsItem *h, handles)
            h->setVisible(value.toBool());
    }

    return CResizablePixmapGraphicsItem::itemChange(change, value);
}

void CRobotGraphicsItem::paint(QPainter *painter,
                               const QStyleOptionGraphicsItem *option,
                               QWidget *widget)
{
    CResizablePixmapGraphicsItem::paint(painter, option, widget);

    foreach (CIRSensor *ir, IRSensors)
    {
        // Start with line which has a zero degree angle
        // (in Qt's counter clockwise system)
        QPointF start(ir->getPosition());
        const qreal scale = getPixmapScale();
        start.rx() *= scale;
        start.ry() *= scale;

        QLineF line(start, start + QPointF(ir->getHitDistance() * scale, 0.0));
        line.setAngle(toCounterClockwiseAngle(ir->getTraceAngle()));
        painter->drawLine(line);
    }

#if 0
    // UNDONE
    QPointF c(boundingRect().center());
    QLineF line(c.x(), 0.0, c.x(), -100.0);

    QPolygonF poly = mapToScene(line.x1(), line.y1(), line.x2(), line.y2());
    QList<QGraphicsItem *> obstacles = scene()->items(poly);

    foreach (QGraphicsItem *ob, obstacles)
    {
        if (ob == this)
            continue;
        QPolygonF obpoly(mapFromItem(ob, ob->boundingRect()));
        for (QPolygonF::iterator it=obpoly.begin(); it!=(obpoly.end()-1); ++it)
        {
            QLineF obl(*it, *(it+1));
            QPointF interp;
            if (line.intersect(obl, &interp) == QLineF::BoundedIntersection)
            {
                QLineF tmp(line.p1(), interp);
                if (tmp.length() < line.length())
                    line.setP2(interp);
            }
        }

    }

    painter->drawLine(line);
#endif
}

void CRobotGraphicsItem::addLED(CLED *l)
{
    LEDs << l;
}

void CRobotGraphicsItem::removeLED(CLED *l)
{
    qDebug() << "Removing LED from robot item";
    LEDs.removeOne(l);
}

void CRobotGraphicsItem::drawLEDs(QPainter *painter) const
{
    const QTransform tr(sceneTransform());
    const qreal scale = getPixmapScale();
    foreach (CLED *l, LEDs)
    {
        if (l->isEnabled())
            drawLED(*painter, l, tr, scale);
    }
}

void CRobotGraphicsItem::addBumper(CBumper *b)
{
    const qreal scale = getPixmapScale();
    QTransform tr;
    tr.scale(scale, scale);

    QGraphicsPolygonItem *pi =
            new QGraphicsPolygonItem(tr.map(b->getPoints()), this);
    pi->setPen(Qt::NoPen);
    pi->setBrush(b->getColor());
    pi->setVisible(false);
    bumperItems[b] = pi;

    bumpers << b;
}

void CRobotGraphicsItem::removeBumper(CBumper *b)
{
    qDebug() << "Removing bumper from robot item";
    delete bumperItems.take(b);
    bumpers.removeOne(b);
}

void CRobotGraphicsItem::addIRSensor(CIRSensor *ir)
{
    IRSensors << ir;
    if (!IRSensorUpdateTimer->isActive())
        IRSensorUpdateTimer->start();
}

void CRobotGraphicsItem::removeIRSensor(CIRSensor *ir)
{
    qDebug() << "Removing IR sensor from robot item";
    IRSensors.removeOne(ir);
    if (IRSensors.isEmpty())
        IRSensorUpdateTimer->stop();
}
