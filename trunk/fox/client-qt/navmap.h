#ifndef NAVMAP_H
#define NAVMAP_H

#include <QWidget>

class CNavMap: public QWidget
{
public:
    enum EObstacle { OBSTACLE_NONE=0,
                     OBSTACLE_LEFT=1<<0,
                     OBSTACLE_RIGHT=1<<1,
                     OBSTACLE_UP=1<<2,
                     OBSTACLE_DOWN=1<<3 };

    enum EEditMode { EDIT_NONE, EDIT_OBSTACLE, EDIT_START, EDIT_GOAL };

private:
    Q_OBJECT

    struct SCell
    {
        int obstacles;
        bool inPath;
        SCell(void) : obstacles(OBSTACLE_NONE), inPath(false) {}
    };

    struct SScanPoint
    {
        // from: grid cell
        // to: 'real vector'
        QPoint from, to;
        SScanPoint(const QPoint &f, const QPoint &t) : from(f), to(t) { }
    };

    struct SScanConnection
    {
        // From and to are 'real vectors'
        QPoint from, to;
        SScanConnection(const QPoint &f, const QPoint &t) : from(f), to(t) { }
    };

    QVector<QVector<SCell> > grid;
    QList<SScanPoint> scanPoints;
    QList<SScanConnection> scanConnections;
    int realCellSize;
    QPoint startPos, goalPos, robotPos;
    int robotRotation;
    EEditMode editMode;
    bool blockEditMode;
    QPoint currentMouseCell;
    EObstacle currentMouseObstacle;

    int getCellSize(void) const;
    QRect getCellRect(const QPoint &cell) const;
    QPoint getCellFromPos(const QPoint &pos) const;
    QPoint getPosFromRealVec(const QPoint &vec);
    EObstacle getObstacleFromPos(const QPoint &pos, const QPoint &cell);
    void drawObstacle(QRect rect, QPainter &painter, int obstacles);
    void drawStart(const QRect &rect, QPainter &painter);
    void drawGoal(const QRect &rect, QPainter &painter);

protected:
    void paintEvent(QPaintEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void leaveEvent(QEvent *);
    void mouseReleaseEvent(QMouseEvent *event);

public:
    CNavMap(QWidget *parent = 0, Qt::WindowFlags f = 0);

    void setGrid(const QSize &size);
    void setRealCellSize(int size) { realCellSize = size; }
    void expandGrid(int left, int up, int right, int down);
    void setPath(const QList<QPoint> &path);
    void setStart(const QPoint &pos) { startPos = pos; update(); }
    void setGoal(const QPoint &pos) { goalPos = pos; update(); }
    void setRobot(const QPoint &pos);
    void setRobotRotation(int r) { robotRotation = r; update(); }
    void addScanPoint(const QPoint &pos);
    void addScanConnection(const QPoint &start, const QPoint &end);
    void markObstacle(const QPoint &pos, int o);
    void markBlockObstacle(const QPoint &pos);
    QPoint getRobot(void) const { return robotPos; }
    QPoint getStart(void) const { return startPos; }
    QPoint getGoal(void) const { return goalPos; }
    QSize getGridSize(void) const;
    int obstacles(const QPoint &pos) const { return grid[pos.x()][pos.y()].obstacles; }
    bool inGrid(const QPoint &pos) const;

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

public slots:
    void clearCells(void);
    void setEditMode(int mode);
    void setBlockEditMode(bool e) { blockEditMode = e; }
};

#endif // NAVMAP_H
