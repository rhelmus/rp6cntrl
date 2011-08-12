#ifndef ROBOTGRAPHICSITEM_H
#define ROBOTGRAPHICSITEM_H

#include "handlegraphicsitem.h"
#include "resizablepixmapgraphicsitem.h"
#include "rp6simul.h"

class CRobotGraphicsItem : public CResizablePixmapGraphicsItem
{
    Q_OBJECT

    QSize origRobotSize;
    QMap<ELEDType, bool> enabledLEDs;
    int leftPower, rightPower;
    int skipFrames;

    typedef QMap<CHandleGraphicsItem::EHandlePosFlags, QGraphicsItem *> THandleList;
    THandleList handles;
    CHandleGraphicsItem *pressedHandle;

    void addHandle(CHandleGraphicsItem::EHandlePosFlags pos);
    QPointF mapDeltaPos(qreal x, qreal y) const;
    QPointF mapDeltaPos(const QPointF &p) const { return mapDeltaPos(p.x(), p.y()); }
    void tryMove(float lpower, float rpower);
    bool tryDoMove(float rotspeed, QPointF dpos);

protected:
    void advance(int phase);
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

public:
    CRobotGraphicsItem(QGraphicsItem *parent = 0);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);

    void enableLED(ELEDType l, bool e) { enabledLEDs[l] = e; }
    void setLeftMotor(int power) { leftPower = power; }
    void setRightMotor(int power) { rightPower = power; }
    void drawLEDs(QPainter *painter) const;

signals:
    void rotationChanged(qreal);
};

#endif // ROBOTGRAPHICSITEM_H
