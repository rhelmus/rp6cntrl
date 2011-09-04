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
    float cmPerPixel, robotLength;
    QList<CLED *> LEDs;
    QMap<EMotor, int> motorSpeed;
    QMap<EMotor, EMotorDirection> motorDirection;
    QList<CBumper *> bumpers;
    QMap<CBumper *, QGraphicsPolygonItem *> bumperItems;
    QList<CIRSensor *> IRSensors;
    QTimer *sensorUpdateTimer;
    QList<CLightSensor *> lightSensors;
    QPixmap m32Pixmap;
    QSize origM32Size;
    float m32Scale;
    QPointF m32Positions[SLOT_END];
    float m32Rotations[SLOT_END];
    EM32Slot activeM32Slot;
    bool m32PixmapDirty;
    bool isBlocked;
    int skipFrames;

    typedef QMap<CHandleGraphicsItem::EHandlePosFlags, QGraphicsItem *> THandleList;
    THandleList handles;
    CHandleGraphicsItem *pressedHandle;

    void addHandle(CHandleGraphicsItem::EHandlePosFlags pos);
    void updateM32Pixmap(void);
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

    void setCmPerPixel(float c) { cmPerPixel = c * getPixmapScale(); }
    void setRobotLength(float l) { robotLength = l; }
    void setM32Slot(EM32Slot s, const QPointF &p, float r);
    void setM32Scale(float s) { m32Scale = s; m32PixmapDirty = true; }
    void setActiveM32Slot(EM32Slot s) { activeM32Slot = s; m32PixmapDirty = true; }
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
