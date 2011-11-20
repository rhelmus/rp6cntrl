#include "bumper.h"
#include "handlegraphicsitem.h"
#include "irsensor.h"
#include "led.h"
#include "lightgraphicsitem.h"
#include "lightsensor.h"
#include "robotgraphicsitem.h"
#include "simulator.h"
#include "utils.h"

#include <math.h>

#include <QtGui>

namespace {

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

    const SRobotConfigDefinitions config =
            CRP6Simulator::getInstance()->getRobotConfigDefinitions();

    // Necessary correction for RP6 library bug: see motor driver
    const float effspeedtimerbase = 1.1 * (config.speedTimerBase + 2.0);

    const float speedfreq = 1000.0 / effspeedtimerbase;
    const float counts_s = (float)motorspeed * speedfreq;
    const float mm_s = counts_s * config.encoderResolution;
    const float px_s = mm_s / (cmperpx * 10.0);

    // Amount of pixels moved during this frame
    float ret = px_s / 1000.0 * frametime;

    if (dir == MOTORDIR_BWD)
        ret = -ret;

    return ret;
}

}

CRobotGraphicsItem::CRobotGraphicsItem(QGraphicsItem *parent)
    : CRotatablePixmapGraphicsItem(parent), m32Enabled(false), m32Scale(1.0),
      activeM32Slot(SLOT_END), m32PixmapDirty(false), skipFrames(0)
{
    QPixmap pm("../resource/rp6-top.png");
    origRobotSize = pm.size();
    setPixmap(pm.scaledToWidth(120, Qt::SmoothTransformation));

    origM32Size = QImage("../resource/m32-top.png").size();
    m32Rotations[SLOT_FRONT] = m32Rotations[SLOT_BACK] = 0.0;

    setDeletable(false);
    setSnapsToGrid(false);

    connect(this, SIGNAL(posChanged(const QPointF &)), SLOT(updateBumpers()));

    sensorUpdateTimer = new QTimer(this);
    sensorUpdateTimer->setInterval(100);
    connect(sensorUpdateTimer, SIGNAL(timeout()), SLOT(updateSensors()));
}

void CRobotGraphicsItem::updateM32Pixmap()
{
    if (activeM32Slot == SLOT_END)
        return;

    const float w = boundingRect().width() * m32Scale;
    m32Pixmap = QPixmap("../resource/m32-top.png").scaledToWidth(w, Qt::SmoothTransformation);

    const QPointF c(m32Pixmap.rect().center());
    QTransform tr;
    tr.translate(c.x(), c.y());
    tr.rotate(m32Rotations[activeM32Slot]);
    tr.translate(-c.x(), -c.y());
    m32Pixmap = m32Pixmap.transformed(tr, Qt::SmoothTransformation);

    m32PixmapDirty = false;
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

    // Don't move if selected
    if (isSelected())
        return;

    const CRobotScene *rscene = qobject_cast<CRobotScene *>(scene());
    Q_ASSERT(rscene);

    // Delay between calls to this function (frame time)
    const float advdelay = rscene->getRobotAdvanceDelay();

    const float lspeed =
            getRobotFrameSpeed(motorSpeed[MOTOR_LEFT], advdelay,
                               motorDirection[MOTOR_LEFT], cmPerPixel);
    const float rspeed =
            getRobotFrameSpeed(motorSpeed[MOTOR_RIGHT], advdelay,
                               motorDirection[MOTOR_RIGHT], cmPerPixel);

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
    // UNDONE: Somehow correct for positive bias caused by low speed update
    //         frequency of RP6 library code?
    const float rotspeed = (lspeed - rspeed) / 2.0;
    float degspeed = 0.0;
    if (rotspeed != 0.0)
    {
        const float robotlengthpx = robotLength / cmPerPixel;
        const float perimeter = robotlengthpx * M_PI;
        const float pxperdeg = perimeter / 360.0;
        // Correct for changed ROTATION_FACTOR values. The default is 688.
        // Note that this will slightly affect the moving speed.
        const float correction = 688.0f /
                CRP6Simulator::getInstance()->getRobotConfigDefinitions().rotationFactor;
        degspeed = rotspeed / pxperdeg * correction;
    }

    /*
      Try to move with calculated move- and rotation speeds. This may fail if
      the robot collides with any obstacles after having rotated/moved. In
      the case of failure try to unstuck the robot by moving it in
      random close positions.
    */
    const bool blocked =
            !tryDoMove(degspeed, mapDeltaPos(0.0, -movespeed), true);

    if (isBlocked != blocked)
    {
        isBlocked = blocked;
        emit robotBlockedChanged(blocked);
    }

    if (blocked)
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

void CRobotGraphicsItem::updateSensors()
{
    const qreal scale = getPixmapScale();

    foreach (CIRSensor *ir, IRSensors)
    {
        // Start with line which has a zero degree angle
        // (in Qt's counter clockwise system)
        QPointF start(ir->getPosition());       
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

    foreach (CLightSensor *light, lightSensors)
    {
        QPointF position = light->getPosition();
        position.rx() *= scale;
        position.ry() *= scale;
        position = mapToScene(position);

        const CRobotScene *rscene = qobject_cast<CRobotScene *>(scene());
        Q_ASSERT(rscene);
        const float intensity = rscene->getIntensity(position);

        // intensity is in 0.0 .. 2.0 range, sensor in 0 .. 1023
        light->setLight((intensity / 2.0) * 1023.0);
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

void CRobotGraphicsItem::paint(QPainter *painter,
                               const QStyleOptionGraphicsItem *option,
                               QWidget *widget)
{
    CRotatablePixmapGraphicsItem::paint(painter, option, widget);

    if (m32Enabled)
    {
        if (m32PixmapDirty)
            updateM32Pixmap();

        if (!m32Pixmap.isNull()) // null if active slot isn't set yet
            painter->drawPixmap(m32Positions[activeM32Slot], m32Pixmap);
    }

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

void CRobotGraphicsItem::start()
{
}

void CRobotGraphicsItem::stop()
{
    motorSpeed.clear();
    motorDirection.clear();
    update();
}

void CRobotGraphicsItem::setM32Slot(EM32Slot s, const QPointF &p, float r)
{
    m32Positions[s] = p * getPixmapScale();
    m32Rotations[s] = r;
    m32PixmapDirty = true;
}

void CRobotGraphicsItem::addRobotLED(CLED *l)
{
    robotLEDs << l;
}

void CRobotGraphicsItem::removeRobotLED(CLED *l)
{
    robotLEDs.removeOne(l);
}

void CRobotGraphicsItem::drawLEDs(QPainter *painter) const
{
    QTransform tr(sceneTransform());

    if (!m32Enabled || (activeM32Slot != SLOT_FRONT)) // Front m32 hides robot LEDs
    {
        const qreal scale = getPixmapScale();
        foreach (CLED *l, robotLEDs)
        {
            if (l->isEnabled())
                drawLED(*painter, l, tr, scale);
        }
    }

    if (m32Enabled && (activeM32Slot != SLOT_END))
    {
        tr.translate(m32Positions[activeM32Slot].x(), m32Positions[activeM32Slot].y());
        tr.translate(m32Pixmap.width()/2.0, m32Pixmap.height()/2.0);
        tr.rotate(m32Rotations[activeM32Slot]);
        tr.translate(-m32Pixmap.width()/2.0, -m32Pixmap.height()/2.0);

        const qreal scale = (qreal)m32Pixmap.width() / (qreal)origM32Size.width();
        foreach (CLED *l, m32LEDs)
        {
            if (l->isEnabled())
                drawLED(*painter, l, tr, scale);
        }
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
    delete bumperItems.take(b);
    bumpers.removeOne(b);
}

void CRobotGraphicsItem::addIRSensor(CIRSensor *ir)
{
    IRSensors << ir;
    if (!sensorUpdateTimer->isActive())
        sensorUpdateTimer->start();
}

void CRobotGraphicsItem::removeIRSensor(CIRSensor *ir)
{
    IRSensors.removeOne(ir);
    if (IRSensors.isEmpty() && lightSensors.isEmpty())
        sensorUpdateTimer->stop();
}

void CRobotGraphicsItem::addLightSensor(CLightSensor *light)
{
    lightSensors << light;
    if (!sensorUpdateTimer->isActive())
        sensorUpdateTimer->start();
}

void CRobotGraphicsItem::removeLightSensor(CLightSensor *light)
{
    lightSensors.removeOne(light);
    if (IRSensors.isEmpty() && lightSensors.isEmpty())
        sensorUpdateTimer->stop();
}

void CRobotGraphicsItem::addM32LED(CLED *l)
{
    m32LEDs << l;
}

void CRobotGraphicsItem::removeM32LED(CLED *l)
{
    m32LEDs.removeOne(l);
}
