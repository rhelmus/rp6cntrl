#ifndef ROBOTGRAPHICSITEM_H
#define ROBOTGRAPHICSITEM_H

#include "handlegraphicsitem.h"
#include "resizablepixmapgraphicsitem.h"
#include "rp6simul.h"

class QGraphicsPolygonItem;

class CRobotGraphicsItem : public CResizablePixmapGraphicsItem
{
    Q_OBJECT

    QSize origRobotSize;
    QMap<ELEDType, bool> enabledLEDs;
    QMap<EMotor, int> motorSpeed;
    QMap<EMotor, EMotorDirection> motorDirection;
    QMap<EBumper, bool> hitBumpers;
    QMap<EBumper, QGraphicsPolygonItem *> bumperItems;
    int skipFrames;

    typedef QMap<CHandleGraphicsItem::EHandlePosFlags, QGraphicsItem *> THandleList;
    THandleList handles;
    CHandleGraphicsItem *pressedHandle;

    void addHandle(CHandleGraphicsItem::EHandlePosFlags pos);
    void createBumperItems(void);
    QPointF mapDeltaPos(qreal x, qreal y) const;
    QPointF mapDeltaPos(const QPointF &p) const { return mapDeltaPos(p.x(), p.y()); }
    qreal getPixmapScale(void) const;
    bool handleObstacles(bool checkbumpers);
    void tryMove(void);
    bool tryDoMove(float rotspeed, QPointF dpos, bool checkbumpers);

private slots:
    void updateBumpers(void);

protected:
    void advance(int phase);
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

public:
    CRobotGraphicsItem(QGraphicsItem *parent = 0);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);

    void initLua(void) { createBumperItems(); }
    void enableLED(ELEDType l, bool e) { enabledLEDs[l] = e; }
    void drawLEDs(QPainter *painter) const;

public slots:
    void setMotorSpeed(EMotor m, int s) { motorSpeed[m] = s; }
    void setMotorDirection(EMotor m, EMotorDirection d) { motorDirection[m] = d; }

signals:
    void robotMoved(const QPointF &, qreal);
    void rotationChanged(qreal); // Manual rotate
    void bumperChanged(EBumper, bool);
};

#endif // ROBOTGRAPHICSITEM_H
