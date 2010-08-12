#include <QDebug>
#include <QMouseEvent>
#include <QPainter>

#include "navmap.h"

CNavMap::CNavMap(QWidget *parent, Qt::WindowFlags f) : QWidget(parent, f),
                  startPos(-1, -1), goalPos(-1, -1), robotPos(-1, -1),
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
                    painter.fillRect(rect, Qt::white);

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
                        painter.drawRect(rect.adjusted(10, 15, -10, -15));
                        painter.drawText(rect, Qt::AlignCenter, "R");
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
            {
                // Mark neighboring cells
                const QSize gridsize(getGridSize());
                QPoint cell(currentMouseCell.x()-1, currentMouseCell.y()); // Left
                if (cell.x() >= 0)
                    markObstacle(cell, OBSTACLE_RIGHT);

                cell = QPoint(currentMouseCell.x()+1, currentMouseCell.y()); // Right
                if (cell.x() < gridsize.width())
                    markObstacle(cell, OBSTACLE_LEFT);

                cell = QPoint(currentMouseCell.x(), currentMouseCell.y()-1); // Up
                if (cell.y() >= 0)
                    markObstacle(cell, OBSTACLE_DOWN);

                cell = QPoint(currentMouseCell.x(), currentMouseCell.y()+1); // Down
                if (cell.y() < gridsize.height())
                    markObstacle(cell, OBSTACLE_UP);
            }
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

void CNavMap::markObstacle(const QPoint &pos, int o)
{
    grid[pos.x()][pos.y()].obstacles |= o;
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
