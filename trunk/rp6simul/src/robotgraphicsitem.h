#ifndef ROBOTGRAPHICSITEM_H
#define ROBOTGRAPHICSITEM_H

#include "handlegraphicsitem.h"
#include "rotatablepixmapgraphicsitem.h"
#include "rp6simul.h"

class QGraphicsPolygonItem;

class CBumper;
class CIRSensor;
class CLED;
class CLightSensor;

class CRobotGraphicsItem : public CRotatablePixmapGraphicsItem
{
    Q_OBJECT

    QSize origRobotSize;
    float cmPerPixel, robotLength;
    QList<CLED *> robotLEDs;
    QMap<EMotor, int> motorSpeed;
    QMap<EMotor, EMotorDirection> motorDirection;
    QList<CBumper *> bumpers;
    QMap<CBumper *, QGraphicsPolygonItem *> bumperItems;
    QList<CIRSensor *> IRSensors;
    QTimer *sensorUpdateTimer;
    QList<CLightSensor *> lightSensors;
    QPixmap m32Pixmap;
    QSize origM32Size;
    bool m32Enabled;
    float m32Scale;
    QPointF m32Positions[SLOT_END];
    float m32Rotations[SLOT_END];
    EM32Slot activeM32Slot;
    bool m32PixmapDirty;
    QList<CLED *> m32LEDs;
    bool isBlocked;
    int skipFrames;
    bool isCustomBlocked;

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
    void initContextMenu(QMenu *menu);
    bool handleContextMenuAction(QAction *a);

public:
    CRobotGraphicsItem(QGraphicsItem *parent = 0);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);

    void start(void);
    void stop(void);
    void setCmPerPixel(float c) { cmPerPixel = c / getPixmapScale(); }
    void setRobotLength(float l) { robotLength = l; }
    void setM32Enabled(bool e) { m32Enabled = e; }
    void setM32Slot(EM32Slot s, const QPointF &p, float r);
    void setM32Scale(float s) { m32Scale = s; m32PixmapDirty = true; }
    void setActiveM32Slot(EM32Slot s) { activeM32Slot = s; m32PixmapDirty = true; }
    void addRobotLED(CLED *l);
    void removeRobotLED(CLED *l);
    void drawLEDs(QPainter *painter) const;
    void addBumper(CBumper *b);
    void removeBumper(CBumper *b);
    void addIRSensor(CIRSensor *ir);
    void removeIRSensor(CIRSensor *ir);
    void addLightSensor(CLightSensor *light);
    void removeLightSensor(CLightSensor *light);
    void addM32LED(CLED *l);
    void removeM32LED(CLED *l);

public slots:
    void setMotorSpeed(EMotor m, int s) { motorSpeed[m] = s; }
    void setMotorDirection(EMotor m, EMotorDirection d) { motorDirection[m] = d; }

signals:
    void robotMoved(const QPointF &, qreal);
    void robotBlockedChanged(bool);
    void bumperChanged(CBumper *, bool);
    void IRSensorChanged(CIRSensor *, float);
};

#endif // ROBOTGRAPHICSITEM_H