#ifndef ROBOTSCENE_H
#define ROBOTSCENE_H

#include <QGraphicsScene>

class CLight
{
    QPointF pos;
    float intensity;

public:
    CLight(const QPointF &p, float i) : pos(p), intensity(i) { }

    float intensityAt(const QPointF &p) const;
    QPointF position(void) const { return pos; }
};

class CRobotScene : public QGraphicsScene
{
    QList<CLight> lights;

protected:
    virtual void drawForeground(QPainter *painter, const QRectF &rect);

public:
    explicit CRobotScene(QObject *parent = 0);

    void addLight(const QPointF &p, float i) { lights << CLight(p, i); }
    void updateLighting(void);
};

#endif // ROBOTSCENE_H
