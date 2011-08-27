#ifndef ROBOTGRAPHICSITEM_H
#define ROBOTGRAPHICSITEM_H

#include "handlegraphicsitem.h"
#include "resizablepixmapgraphicsitem.h"
#include "rp6simul.h"

class QGraphicsPolygonItem;

class CBumper;
class CIRSensor;
class CLED;
class CLightSensor;

class CRobotGraphicsItem : public CResizablePixmapGraphicsItem
{
    Q_OBJECT

    QSize origRobotSize;
    QList<CLED *> LEDs;
    QMap<EMotor, int> motorSpeed;
    QMap<EMotor, EMotorDirection> motorDirection;
    QList<CBumper *> bumpers;
    QMap<CBumper *, QGraphicsPolygonItem *> bumperItems;
    QList<CIRSensor *> IRSensors;
    QTimer *sensorUpdateTimer;
    QList<CLightSensor *> lightSensors;
    bool isBlocked;
    int skipFrames;

    typedef QMap<CHandleGraphicsItem::EHandlePosFlags, QGraphicsItem *> THandleList;
    THandleList handles;
    CHandleGraphicsItem *pressedHandle;

    void addHandle(CHandleGraphicsItem::EHandlePosFlags pos);
    QPointF mapDeltaPos(qreal x, qreal y) const;
    QPointF mapDeltaPos(const QPointF &p) const { return mapDeltaPos(p.x(), p.y()); }
    qreal getPixmapScale(void) const;
    bool handleObstacles(bool checkbumpers);
    void tryMove(void);
    bool tryDoMove(float rotspeed, QPointF dpos, bool checkbumpers);

private slots:
    void updateBumpers(void);
    void updateSensors(void);

protected:
    void advance(int phase);
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

public:
    CRobotGraphicsItem(QGraphicsItem *parent = 0);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);

    void addLED(CLED *l);
    void removeLED(CLED *l);
    void drawLEDs(QPainter *painter) const;
    void addBumper(CBumper *b);
    void removeBumper(CBumper *b);
    void addIRSensor(CIRSensor *ir);
    void removeIRSensor(CIRSensor *ir);
    void addLightSensor(CLightSensor *light);
    void removeLightSensor(CLightSensor *light);

public slots:
    void setMotorSpeed(EMotor m, int s) { motorSpeed[m] = s; }
    void setMotorDirection(EMotor m, EMotorDirection d) { motorDirection[m] = d; }

signals:
    void robotMoved(const QPointF &, qreal);
    void robotBlockedChanged(bool);
    void rotationChanged(qreal); // Manual rotate
    void bumperChanged(CBumper *, bool);
    void IRSensorChanged(CIRSensor *, float);
};

#endif // ROBOTGRAPHICSITEM_H
