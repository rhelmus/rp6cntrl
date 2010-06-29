#include <QDebug>
#include <QMouseEvent>
#include <QPainter>

#include "navmap.h"

CNavMap::CNavMap(QWidget *parent, Qt::WindowFlags f) : QWidget(parent, f), robotPos(-1, -1), addObstacleMode(false)
{
}

int CNavMap::getCellSize() const
{
    const QSize gridSize = getGridSize();
    const int widthSize = width() / gridSize.width();
    const int heightSize = height() / gridSize.height();
    return std::min(widthSize, heightSize);
}

QRect CNavMap::getCellRect(int x, int y) const
{
    const int size = getCellSize();
    const int xpos = x * size, ypos = y * size;
    return QRect(xpos, ypos, size, size);
}

QPoint CNavMap::getCellFromPos(const QPoint &pos) const
{
    const int size = getCellSize();
    return QPoint(pos.x() / size, pos.y() / size);
}

void CNavMap::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.save();

    painter.setRenderHint(QPainter::Antialiasing, true);

    if (!grid.isEmpty())
    {
        const QSize size = getGridSize();

        for (int x=0; x<size.width(); ++x)
        {
            for (int y=0; y<size.height(); ++y)
            {
                QRect rect = getCellRect(x, y);

                painter.setPen(QPen(Qt::gray, 1));
                painter.drawRect(rect);

                if ((robotPos.x() == x) && (robotPos.y() == y))
                {
                    painter.setPen(QPen(Qt::red, 3));
                    painter.drawEllipse(rect.adjusted(10, 10, -10, -10));
                    painter.drawText(rect, Qt::AlignCenter, "R");
                }

                painter.setPen(QPen(Qt::black, 1));
                for (QList<QPoint>::iterator it=grid[x][y].connections.begin();
                     it!=grid[x][y].connections.end(); ++it)
                {
                    QRect otherRect = getCellRect(it->x(), it->y());
                    painter.drawLine(rect.center(), otherRect.center());
                }

                if (grid[x][y].isObstacle ||
                    (addObstacleMode && (currentObstacleMousePos.x() == x) && (currentObstacleMousePos.y() == y)))
                    painter.fillRect(rect, Qt::blue);

            }
        }
    }

    painter.restore();
}

void CNavMap::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pos = getCellFromPos(event->pos());

    if (pos != currentObstacleMousePos)
    {
        currentObstacleMousePos = pos;
        update();
    }

    event->accept();
}

void CNavMap::mouseReleaseEvent(QMouseEvent *event)
{
    if (addObstacleMode && (event->button() == Qt::LeftButton))
    {
        markObstacle(currentObstacleMousePos);
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
    update();
}

void CNavMap::setRobot(const QPoint &pos)
{
    robotPos = pos;
    update();
}

void CNavMap::markObstacle(const QPoint &pos)
{
    grid[pos.x()][pos.y()].isObstacle = true;
}

void CNavMap::connectCells(const QPoint &pos1, const QPoint &pos2)
{
    grid[pos1.x()][pos1.y()].connections.push_back(pos2);
}

QSize CNavMap::getGridSize() const
{
    if (grid.isEmpty())
        return QSize(0, 0);

    return QSize(grid.size(), grid[0].size());
}

void CNavMap::clearConnections()
{
    if (grid.isEmpty())
        return;

    const QSize size = getGridSize();
    for (int w=0; w<size.width(); ++w)
    {
        for (int h=0; h<size.height(); ++h)
        {
            grid[w][h].connections.clear();
        }
    }

    update();
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
            grid[w][h].connections.clear();
            grid[w][h].isObstacle = false;
        }
    }

    update();
}

void CNavMap::toggleObstacleAdd(bool e)
{
    addObstacleMode = e;
    setMouseTracking(e);
}
