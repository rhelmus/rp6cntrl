#ifndef ROBOTSCENE_H
#define ROBOTSCENE_H

#include <QGraphicsScene>
#include <QHash>

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
    QHash<QGraphicsItem *, bool> walls;
    const QSizeF blockSize;
    QPixmap blockPixmap;
    QPixmap backGroundPixmap;
    QImage shadowImage;
    QPointF mousePos, mouseDragStartPos;

    enum EMouseMode { MODE_POINT, MODE_WALL };
    EMouseMode mouseMode;

    QRectF getDragRect(void) const;

protected:
    void drawBackground(QPainter *painter, const QRectF &rect);
    void drawForeground(QPainter *painter, const QRectF &rect);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

public:
    explicit CRobotScene(QObject *parent = 0);

    void addLight(const QPointF &p, float i) { lights << CLight(p, i); }
    void addWall(const QRectF &rect, bool st);
    void addWall(qreal x, qreal y, qreal w, qreal h, bool st)
    { addWall(QRectF(x, y, w, h), st); }
    void updateShadows(void);
};

#endif // ROBOTSCENE_H
