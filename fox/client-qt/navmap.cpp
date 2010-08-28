#include <QDebug>
#include <QMouseEvent>
#include <QPainter>

#include "navmap.h"

CNavMap::CNavMap(QWidget *parent, Qt::WindowFlags f) : QWidget(parent, f),
                  startPos(-1, -1), goalPos(-1, -1), robotPos(-1, -1), robotRotation(0),
                  editMode(EDIT_NONE), blockEditMode(false)
{
}

int CNavMap::getCellSize() const
{
    const QSize gridSize = getGridSize();
    const int widthSize = width() / gridSize.width();
    const int heightSize = height() / gridSize.height();
    return std::min(widthSize, heightSize);
}

QRect CNavMap::getCellRect(const QPoint &cell) const
{
    const int size = getCellSize();
    const int xpos = cell.x() * size, ypos = cell.y() * size;
    return QRect(xpos, ypos, size, size);
}

QPoint CNavMap::getCellFromPos(const QPoint &pos) const
{
    const int size = getCellSize();
    return QPoint(pos.x() / size, pos.y() / size);
}

CNavMap::EObstacle CNavMap::getObstacleFromPos(const QPoint &pos, const QPoint &cell)
{
    const QRect rect(getCellRect(cell));
    const int xdiff = pos.x() - rect.x(), ydiff = pos.y() - rect.y();
    const int wdiff = (rect.x() + rect.width()) - pos.x(), hdiff = (rect.y() + rect.height()) - pos.y();
    const int mindiff = std::min(std::min(xdiff, ydiff), std::min(wdiff, hdiff));

    if (mindiff == xdiff)
        return OBSTACLE_LEFT;
    else if (mindiff == ydiff)
        return OBSTACLE_UP;
    else if (mindiff == wdiff)
        return OBSTACLE_RIGHT;
    else
        return OBSTACLE_DOWN;
}

void CNavMap::drawObstacle(QRect rect, QPainter &painter, int obstacles)
{
    painter.setPen(QPen(Qt::blue, 3));

    // Decrease rect to make obstacles look more inward
    rect.adjust(3, 3, -3, -3);

    if (obstacles & OBSTACLE_LEFT)
        painter.drawLine(rect.topLeft(), rect.bottomLeft());
    if (obstacles & OBSTACLE_RIGHT)
        painter.drawLine(rect.topRight(), rect.bottomRight());
    if (obstacles & OBSTACLE_UP)
        painter.drawLine(rect.topLeft(), rect.topRight());
    if (obstacles & OBSTACLE_DOWN)
        painter.drawLine(rect.bottomLeft(), rect.bottomRight());
}

void CNavMap::drawStart(const QRect &rect, QPainter &painter)
{
    painter.setPen(QPen(Qt::green, 3));
    painter.drawEllipse(rect.adjusted(10, 10, -10, -10));
    painter.drawText(rect, Qt::AlignCenter, "S");
}

void CNavMap::drawGoal(const QRect &rect, QPainter &painter)
{
    painter.setPen(QPen(Qt::red, 3));
    painter.drawEllipse(rect.adjusted(10, 10, -10, -10));
    painter.drawText(rect, Qt::AlignCenter, "G");
}

void CNavMap::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.save();

    painter.setRenderHint(QPainter::Antialiasing, true);

    if (!grid.isEmpty())
    {
        const QSize size = getGridSize();
        const QRect uprect(event->rect());
        const QPoint start(getCellFromPos(QPoint(uprect.x(), uprect.y())));
        QPoint end(getCellFromPos(QPoint(uprect.x() + uprect.width(), uprect.y() + uprect.height())));

        if ((end.x() >= size.width()) || (end.y() >= size.height()))
            end = QPoint(size.width()-1, size.height()-1);

        for (int x=start.x(); x<=end.x(); ++x)
        {
            for (int y=start.y(); y<=end.y(); ++y)
            {
                QRect rect = getCellRect(QPoint(x, y));

                if (blockEditMode && (editMode == EDIT_OBSTACLE) &&
                    (currentMouseCell.x() == x) && (currentMouseCell.y() == y))
                    painter.fillRect(rect, Qt::blue);
                else
                {
                    painter.setPen(QPen(Qt::gray, 1));
                    painter.drawRect(rect);
                    painter.fillRect(rect, (grid[x][y].inPath) ? Qt::green : Qt::white);

                    if ((editMode != EDIT_NONE) && (currentMouseCell.x() == x) && (currentMouseCell.y() == y))
                    {
                        switch(editMode)
                        {
                        case EDIT_OBSTACLE: drawObstacle(rect, painter, currentMouseObstacle); break;
                        case EDIT_START: drawStart(rect, painter); break;
                        case EDIT_GOAL: drawGoal(rect, painter); break;
                        default: break;
                        }
                    }
                    else if ((startPos.x() == x) && (startPos.y() == y))
                        drawStart(rect, painter);
                    else if ((goalPos.x() == x) && (goalPos.y() == y))
                        drawGoal(rect, painter);

                    if ((robotPos.x() == x) && (robotPos.y() == y))
                    {
                        painter.setPen(QPen(Qt::black, 3));
                        const QRect r(rect.adjusted(12, 8, -12, -8));

                        if (robotRotation)
                        {
                            // From http://wiki.forum.nokia.com/index.php/CS001514_-_Rotate_picture_in_Qt
                            const QPoint c(r.center());
                            painter.translate(c);
                            painter.rotate(robotRotation);
                            painter.translate(-c);
                        }

                        painter.drawRect(r);
                        painter.drawText(rect, Qt::AlignCenter, "R");
                        painter.resetTransform();
                    }

                    if (grid[x][y].obstacles != OBSTACLE_NONE)
                        drawObstacle(rect, painter, grid[x][y].obstacles);
                }
            }
        }
    }

    painter.restore();
}

void CNavMap::mouseMoveEvent(QMouseEvent *event)
{
    QPoint cell(getCellFromPos(event->pos()));
    if (!inGrid(cell))
    {
        currentMouseCell.setX(-1); // Invalidate
        return;
    }

    currentMouseCell = cell;

    if (!blockEditMode)
        currentMouseObstacle = getObstacleFromPos(event->pos(), cell);

    update();
    event->accept();
}

void CNavMap::leaveEvent(QEvent *)
{
    currentMouseCell.setX(-1); // Invalidate
    update();
}

void CNavMap::mouseReleaseEvent(QMouseEvent *event)
{
    if ((editMode != EDIT_NONE) && (event->button() == Qt::LeftButton))
    {
        if (editMode == EDIT_OBSTACLE)
        {
            if (blockEditMode)
                markBlockObstacle(currentMouseCell);
            else
                markObstacle(currentMouseCell, currentMouseObstacle);

        }
        else if (editMode == EDIT_START)
            setStart(currentMouseCell);
        else if (editMode == EDIT_GOAL)
            setGoal(currentMouseCell);

        event->accept();
    }
    else
        event->ignore();
}

void CNavMap::setGrid(const QSize &size)
{
    grid.clear();
    grid.resize(size.width());
    grid.fill(QVector<SCell>(size.height()));
    adjustSize();
}

void CNavMap::expandGrid(int left, int up, int right, int down)
{
    QSize gridsize(getGridSize());

    if (left)
    {
        grid.insert(0, left, QVector<SCell>(gridsize.height()));
        gridsize.rwidth() += left;
        startPos.rx() += left;
        goalPos.rx() += left;
        robotPos.rx() += left;
    }

    if (up)
    {
        gridsize.rheight() += up;

        for (int x=0; x<gridsize.width(); ++x)
            grid[x].insert(0, up, SCell());

        startPos.ry() += up;
        goalPos.ry() += up;
        robotPos.ry() += up;
    }

    if (right)
    {
        grid.insert(grid.end(), right, QVector<SCell>(gridsize.height()));
        gridsize.rwidth() += right;
    }

    if (down)
    {
        gridsize.rheight() += down;

        for (int x=0; x<gridsize.width(); ++x)
            grid[x].insert(grid[x].end(), down, SCell());
    }

    adjustSize();
}

void CNavMap::setPath(const QList<QPoint> &path)
{
    const QSize size(getGridSize());

    // First reset all cells
    for (int x=0; x<size.width(); ++x)
    {
        for (int y=0; y<size.height(); ++y)
            grid[x][y].inPath = false;
    }

    foreach(QPoint cell, path)
    {
        grid[cell.x()][cell.y()].inPath = true;
    }

    update();
}

void CNavMap::setRobot(const QPoint &pos)
{
    robotPos = pos;
    grid[pos.x()][pos.y()].inPath = false;
    update();
}

void CNavMap::markObstacle(const QPoint &pos, int o)
{
    grid[pos.x()][pos.y()].obstacles |= o;
    update();
}

void CNavMap::markBlockObstacle(const QPoint &pos)
{
    grid[pos.x()][pos.y()].obstacles = OBSTACLE_LEFT | OBSTACLE_RIGHT |
                                       OBSTACLE_UP | OBSTACLE_DOWN;

    // Mark neighboring cells
    const QSize gridsize(getGridSize());
    QPoint neighbour(pos.x()-1, pos.y()); // Left
    if (neighbour.x() >= 0)
        markObstacle(neighbour, OBSTACLE_RIGHT);

    neighbour = QPoint(pos.x()+1, pos.y()); // Right
    if (neighbour.x() < gridsize.width())
        markObstacle(neighbour, OBSTACLE_LEFT);

    neighbour = QPoint(pos.x(), pos.y()-1); // Up
    if (neighbour.y() >= 0)
        markObstacle(neighbour, OBSTACLE_DOWN);

    neighbour = QPoint(pos.x(), pos.y()+1); // Down
    if (neighbour.y() < gridsize.height())
        markObstacle(neighbour, OBSTACLE_UP);
}

QSize CNavMap::getGridSize() const
{
    if (grid.isEmpty())
        return QSize(0, 0);

    return QSize(grid.size(), grid[0].size());
}

bool CNavMap::inGrid(const QPoint &pos) const
{
    const QSize size = getGridSize();
    return ((pos.x() >= 0) && (pos.x() < size.width()) &&
            (pos.y() >= 0) && (pos.y() < size.height()));
}

QSize CNavMap::minimumSizeHint() const
{
    const int mincellsize = 40;
    const QSize gridsize(getGridSize());

    if (gridsize.isNull())
        return QSize(250, 250);

    return gridsize * mincellsize;
}

QSize CNavMap::sizeHint() const
{
    const int cellsize = 60;
    const QSize gridsize(getGridSize());

    if (gridsize.isNull())
        return QSize(400, 400);

    return gridsize * cellsize;
}

void CNavMap::clearCells()
{
    if (grid.isEmpty())
        return;

    const QSize size = getGridSize();
    for (int w=0; w<size.width(); ++w)
    {
        for (int h=0; h<size.height(); ++h)
        {
            grid[w][h].obstacles = OBSTACLE_NONE;
        }
    }

    update();
}

void CNavMap::setEditMode(int mode)
{
    editMode = static_cast<EEditMode>(mode);
    setMouseTracking(editMode != EDIT_NONE);
}
