#include <iostream>
#include <assert.h>
#include <math.h>

#include <QDebug>
#include <QTime>

#include "pathengine.h"

CPathEngine::CPathEngine() : startCell(NULL), goalCell(NULL)
{
}

QSize CPathEngine::getGridSize() const
{
    if (grid.isEmpty())
        return QSize(0, 0);

    return QSize(grid.size(), grid[0].size());
}

int CPathEngine::getCellLength(const SCell *c1, const SCell *c2) const
{
    // Manhatten distance
    return abs(c1->xPos - c2->xPos) + abs(c1->yPos - c2->yPos);
//    const float xc = (float)(c1->xPos - c2->xPos) * (float)(c1->xPos - c2->xPos);
//    const float yc = (float)(c1->yPos - c2->yPos) * (float)(c1->yPos - c2->yPos);
//    return sqrt(xc + yc);
}

CPathEngine::EConnection CPathEngine::getCellConnection(const SCell *c1, const SCell *c2) const
{
    if (c1->connections[CONNECTION_LEFT] == c2)
        return CONNECTION_LEFT;
    if (c1->connections[CONNECTION_RIGHT] == c2)
        return CONNECTION_RIGHT;
    if (c1->connections[CONNECTION_UP] == c2)
        return CONNECTION_UP;
    if (c1->connections[CONNECTION_DOWN] == c2)
        return CONNECTION_DOWN;

    assert(false); // This really should not happen
    return CONNECTION_DOWN; // Dummy ret
}

int CPathEngine::pathScore(const SCell *c1, const SCell *c2) const
{
    // Score is based on if the bot has to turn: we prefer to travel in straight lines
    int ret = 1; // min 1

    if (c1->parent)
    {
        EConnection first = getCellConnection(c1, c2), parent = getCellConnection(c1->parent, c1);
        if (first != parent) // Changing direction?
            ret+=10; // Increase score
    }

    return ret;
}

void CPathEngine::setGrid(const QSize &size)
{
    grid.clear();
    grid.resize(size.width());
    grid.fill(QVector<SCell>(size.height()));

    // Fill in coords connections
    for (int x=0; x<size.width(); ++x)
    {
        for (int y=0; y<size.height(); ++y)
        {
            grid[x][y].xPos = x;
            grid[x][y].yPos = y;

            if ((x-1) >= 0)
                grid[x][y].connections[CONNECTION_LEFT] = &grid[x-1][y];
            if ((x+1) < size.width())
                grid[x][y].connections[CONNECTION_RIGHT] = &grid[x+1][y];
            if ((y-1) >= 0)
                grid[x][y].connections[CONNECTION_UP] = &grid[x][y-1];
            if ((y+1) < size.height())
                grid[x][y].connections[CONNECTION_DOWN] = &grid[x][y+1];
        }
    }
}

void CPathEngine::initPath(const QPoint &start, const QPoint &goal)
{
    startCell = &grid[start.x()][start.y()];
    goalCell = &grid[goal.x()][goal.y()];

    const QSize gridsize(getGridSize());
    for (int x=0; x<gridsize.width(); ++x)
    {
        for (int y=0; y<gridsize.height(); ++y)
        {
            grid[x][y].score = 0;
            grid[x][y].distCost = 0;
            grid[x][y].open = grid[x][y].closed = false;
            grid[x][y].parent = NULL;
        }
    }

    openList.clear();
    startCell->open = true;
    startCell->distCost = getCellLength(startCell, goalCell);

#ifdef USE_QTMAP
    openList.insert(startCell->distCost, startCell);
#else
    openList.insert(startCell);
#endif
}

bool CPathEngine::calcPath(QList<QPoint> &output)
{
    const QSize gridsize = getGridSize();

    QTime starttime;
    starttime.start();

#ifdef USE_QTMAP
    while (!openList.isEmpty())
#else
    while (!openList.empty())
#endif
    {
#ifdef USE_QTMAP
        SCell *cell = openList.begin().value();
#else
        SCell *cell = *openList.begin();
#endif
        openList.erase(openList.begin());

        cell->open = false;
        cell->closed = true;

        if (cell == goalCell)
        {
            SCell *c = cell;
            while (c)
            {
                output.push_front(QPoint(c->xPos, c->yPos));
                c = c->parent;
            }
            qDebug() << "Path done: " << starttime.elapsed() << " ms";
//            for (int y=0; y<gridsize.height(); ++y)
//            {
//                for (int x=0; x<gridsize.width(); ++x)
//                {
//                    std::cout << grid[x][y].distCost << ",";
//                }
//                std::cout << '\n';
//            }
//            std::cout << '\n' << std::flush;
            return true;
        }

        for (int i=0; i<MAX_CONNECTIONS; ++i)
        {
            if (!cell->connections[i] || cell->connections[i]->closed)
                continue;

            SCell *child = cell->connections[i];
            int newscore = cell->score + pathScore(cell, child);

            if (child->open /*|| child->closed*/)
            {
                if ((newscore >= child->score))
                    continue;

                // Remove any present in open list so we can change the distCost
#ifdef USE_QTMAP
                TOpenList::iterator it = openList.lowerBound(child->distCost);
                assert(it != openList.end());
                while ((it != openList.end()) && (it.key() == child->distCost))
                {
                    if (it.value() == child)
                    {
                        openList.erase(it);
                        break; // Assume child cannot be present more than once
                    }
                    ++it;
                }
#else
                std::pair<TOpenList::iterator, TOpenList::iterator> itpair = openList.equal_range(child);
                if ((itpair.first != openList.end()) && (itpair.second != openList.end()))
                {
                    TOpenList::iterator it = itpair.first;
                    while ((*it)->distCost == child->distCost)
                    {
                        if (*it == child)
                        {
                            // Assume only 1 duplicate can exist
                            openList.erase(it);
                            break;
                        }
                        ++it;
                    }
                }
#endif
            }

            child->parent = cell;
            child->score = newscore;
            child->closed = false;

            child->distCost = child->score + getCellLength(child, goalCell);
            child->open = true;

#ifdef USE_QTMAP
            openList.insert(child->distCost, child);
#else
            openList.insert(child);
#endif
        }

    }

    qDebug() << "Failed to generate path!";
    qDebug() << "Time: " << starttime.elapsed();

    return false;
}

void CPathEngine::breakConnection(const QPoint &cell, EConnection connection)
{
    grid[cell.x()][cell.y()].connections[connection] = NULL;
}
