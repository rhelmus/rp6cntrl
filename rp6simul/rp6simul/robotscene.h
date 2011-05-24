#ifndef ROBOTSCENE_H
#define ROBOTSCENE_H

#include <QGraphicsScene>
#include <QHash>

class CLightGraphicsItem;
class CRobotGraphicsItem;

class CRobotScene : public QGraphicsScene
{
public:
    enum EMouseMode { MODE_POINT, MODE_WALL, MODE_BOX, MODE_LIGHT, MODE_DELETE };

private:
    Q_OBJECT

    struct SOldLightSettings
    {
        QPointF pos;
        float radius;
        SOldLightSettings(void) : radius(0.0) { }
        SOldLightSettings(const QPointF &p, float r) : pos(p), radius(r) { }
    };

    QList<CLightGraphicsItem *> lights;
    QHash<QGraphicsItem *, bool> walls;
    CRobotGraphicsItem *robotGraphicsItem;
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
    QRectF getLightDragRect(void) const;

private slots:
    void removeLight(QObject *o);
    void removeWall(QObject *o);
    void markLightingDirty(void) { if (editModeEnabled) lightingDirty = true; }

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
    CRobotGraphicsItem *getRobotItem(void) const { return robotGraphicsItem; }

public slots:
    void clearMap(void);
};

#endif // ROBOTSCENE_H
