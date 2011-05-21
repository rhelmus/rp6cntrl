#ifndef ROBOTSCENE_H
#define ROBOTSCENE_H

#include <QGraphicsScene>
#include <QHash>

class CLightGraphicsItem;

class CLight
{
    QPointF pos;
    float radius;

public:
    CLight(const QPointF &p, float r) : pos(p), radius(r) { }

    float intensityAt(const QPointF &p,
                      const QList<QPolygonF> &obstacles) const;
    QPointF position(void) const { return pos; }
};

class CRobotScene : public QGraphicsScene
{
public:
    enum EMouseMode { MODE_POINT, MODE_WALL, MODE_BOX, MODE_LIGHT, MODE_DELETE };

private:
    Q_OBJECT

    QList<CLightGraphicsItem *> lights;
    QHash<QGraphicsItem *, bool> walls;
    QHash<QGraphicsItem *, QPointF> oldWallPositions;
    const QSizeF blockSize;
    QPixmap blockPixmap, boxPixmap;
    QPixmap backGroundPixmap;
    QImage shadowImage, lightImage;
    bool lightingDirty;
    QPointF mousePos, mouseDragStartPos;    
    bool dragging;
    EMouseMode mouseMode;
    bool editModeEnabled;

    QRectF getDragRect(void) const;

protected:
    void drawBackground(QPainter *painter, const QRectF &rect);
    void drawForeground(QPainter *painter, const QRectF &rect);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

public:
    explicit CRobotScene(QObject *parent = 0);

    void addLight(const QPointF &p, float r);
    void addWall(const QRectF &rect, bool st);
    void addWall(qreal x, qreal y, qreal w, qreal h, bool st)
    { addWall(QRectF(x, y, w, h), st); }
    void updateLighting(void);
    void setMouseMode(EMouseMode mode);
    void setEditModeEnabled(bool e);
};

#endif // ROBOTSCENE_H
