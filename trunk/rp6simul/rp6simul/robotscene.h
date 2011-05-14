#ifndef ROBOTSCENE_H
#define ROBOTSCENE_H

#include <QGraphicsScene>

class QImage;

class CLight
{
    QPointF pos;
    float radius;

public:
    CLight(const QPointF &p, float r) : pos(p), radius(r) { }

    float intensityAt(const QPointF &p) const;
    QPointF position(void) const { return pos; }
    float fullRadius(void) const { return radius; }
    float intenseRadius(void) const { return radius * 0.5; }
};

class CRobotScene : public QGraphicsScene
{
    QList<CLight> lights;
    QList<QGraphicsItem *> wallBlocks;
    const QSizeF blockSize;
    QPixmap blockPixmap;
    QPixmap backGroundPixmap;
    QImage shadowImage;

protected:
    void drawBackground(QPainter *painter, const QRectF &rect);
    void drawForeground(QPainter *painter, const QRectF &rect);

public:
    explicit CRobotScene(QObject *parent = 0);

    void addLight(const QPointF &p, float i) { lights << CLight(p, i); }
    void addVertWall(const QPointF &pos, float h);
    void addHorizWall(const QPointF &pos, float w);
    void updateShadows(void);
};

#endif // ROBOTSCENE_H
