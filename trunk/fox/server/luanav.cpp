#include <assert.h>

#include <QtGui/QVector2D>
#include <QDebug>

#include "luanav.h"
#include "pathengine.h"

namespace NLuaNav
{

// Lua vector

int vecNew(lua_State *l)
{
    QVector2D *vec = NULL;

    if (lua_isuserdata(l, 1)) // Construct with another vector
        vec = new QVector2D(*NLua::checkClassData<QVector2D>(l, 1, "vector"));
    else // Construct with given x&y
        vec = new QVector2D(luaL_optint(l, 1, 0), luaL_optint(l, 2, 0));

    assert(vec);
    NLua::createClass(l, vec, "vector", vecDel);
    return 1;
}

int vecDel(lua_State *l)
{
    qDebug() << "Removing vector";
    delete NLua::checkClassData<QVector2D>(l, 1, "vector");
    return 0;
}

int vecGetX(lua_State *l)
{
    QVector2D *vec = NLua::checkClassData<QVector2D>(l, 1, "vector");
    lua_pushnumber(l, vec->x());
    return 1;
}

int vecSetX(lua_State *l)
{
    QVector2D *vec = NLua::checkClassData<QVector2D>(l, 1, "vector");
    vec->setX(luaL_checknumber(l, 2));
    return 0;
}

int vecGetY(lua_State *l)
{
    QVector2D *vec = NLua::checkClassData<QVector2D>(l, 1, "vector");
    lua_pushnumber(l, vec->y());
    return 1;
}

int vecSetY(lua_State *l)
{
    QVector2D *vec = NLua::checkClassData<QVector2D>(l, 1, "vector");
    vec->setY(luaL_checknumber(l, 2));
    return 0;
}

int vecSet(lua_State *l)
{
    QVector2D *vec = NLua::checkClassData<QVector2D>(l, 1, "vector");
    vec->setX(luaL_checknumber(l, 2));
    vec->setY(luaL_checknumber(l, 3));
    return 0;
}

int vecOperator(lua_State *l)
{
    const char *op = static_cast<const char *>(lua_touserdata(l, lua_upvalueindex(1)));

    QVector2D *ret = NULL;
    if (!strcmp(op, "+") || !strcmp(op, "-"))
    {
        QVector2D *vec1 = NLua::checkClassData<QVector2D>(l, 1, "vector");
        QVector2D *vec2 = NLua::checkClassData<QVector2D>(l, 2, "vector");

        if (!strcmp(op, "+"))
            ret = new QVector2D(*vec1 + *vec2);
        else
            ret = new QVector2D(*vec1 - *vec2);
    }
    else if (!strcmp(op, "*"))
    {
        if (lua_isnumber(l, 1))
        {
            QVector2D *vec = NLua::checkClassData<QVector2D>(l, 2, "vector");
            ret = new QVector2D(lua_tonumber(l, 1) * *vec);
        }
        else
        {
            QVector2D *vec = NLua::checkClassData<QVector2D>(l, 1, "vector");
            if (lua_isnumber(l, 2))
                ret = new QVector2D(*vec * lua_tonumber(l, 2));
            else
            {
                QVector2D *vec2 = NLua::checkClassData<QVector2D>(l, 2, "vector");
                ret = new QVector2D(*vec * *vec2);
            }
        }
    }
    else if (!strcmp(op, "/"))
    {
        QVector2D *vec = NLua::checkClassData<QVector2D>(l, 1, "vector");
        ret = new QVector2D(*vec / luaL_checknumber(l, 2));
    }

    assert(ret);
    NLua::createClass(l, ret, "vector", vecDel);
    return 1;
}

int vecEqual(lua_State *l)
{
    QVector2D *vec1 = NLua::checkClassData<QVector2D>(l, 1, "vector");
    QVector2D *vec2 = NLua::checkClassData<QVector2D>(l, 2, "vector");
    lua_pushboolean(l, *vec1 == *vec2);
    return 1;
}

int vecLength(lua_State *l)
{
    QVector2D *vec = NLua::checkClassData<QVector2D>(l, 1, "vector");
    lua_pushnumber(l, vec->length());
    return 1;
}

int vecNormalize(lua_State *l)
{
    QVector2D *vec = NLua::checkClassData<QVector2D>(l, 1, "vector");
    vec->normalize();
    return 0;
}

int vecToString(lua_State *l)
{
    QVector2D *vec = NLua::checkClassData<QVector2D>(l, 1, "vector");
    lua_pushfstring(l, "vector(%f, %f)", vec->x(), vec->y());
    return 1;
}

// Lua path engine

int pathENew(lua_State *l)
{
    NLua::createClass(l, new CPathEngine, "pathengine", pathEDel);
    return 1;
}

int pathEDel(lua_State *l)
{
    qDebug() << "Removing pathengine";
    delete NLua::checkClassData<CPathEngine>(l, 1, "pathengine");
    return 0;
}

int pathESetGrid(lua_State *l)
{
    CPathEngine *pe = NLua::checkClassData<CPathEngine>(l, 1, "pathengine");
    const int w = luaL_checkint(l, 2), h = luaL_checkint(l, 3);
    pe->setGrid(QSize(w, h));
    return 0;
}

int pathEExpandGrid(lua_State *l)
{
    CPathEngine *pe = NLua::checkClassData<CPathEngine>(l, 1, "pathengine");
    const int left = luaL_checkint(l, 2);
    const int up = luaL_checkint(l, 3);
    const int right = luaL_checkint(l, 4);
    const int down = luaL_checkint(l, 5);
    pe->expandGrid(left, up, right, down);
    return 0;
}

int pathESetObstacle(lua_State *l)
{
    CPathEngine *pe = NLua::checkClassData<CPathEngine>(l, 1, "pathengine");
    const int x = luaL_checkint(l, 2), y = luaL_checkint(l, 3);
    pe->breakAllConnections(QPoint(x, y));
    return 0;
}

int pathEInitPath(lua_State *l)
{
    CPathEngine *pe = NLua::checkClassData<CPathEngine>(l, 1, "pathengine");
    const QPoint start(luaL_checkint(l, 2), luaL_checkint(l, 3));
    const QPoint goal(luaL_checkint(l, 4), luaL_checkint(l, 5));
    pe->initPath(start, goal);
    return 0;
}

int pathECalcPath(lua_State *l)
{
    CPathEngine *pe = NLua::checkClassData<CPathEngine>(l, 1, "pathengine");
    QList<QPoint> path;
    if (pe->calcPath(path))
    {
        lua_pushboolean(l, true); // Success

        const int size = path.size();

        lua_newtable(l); // Returning path array
        const int tab = lua_gettop(l);

        for (int i=1; i<=size; ++i)
        {
            lua_newtable(l); // X&Y pair

            lua_pushinteger(l, path[i-1].x());
            lua_setfield(l, -2, "x");

            lua_pushinteger(l, path[i-1].y());
            lua_setfield(l, -2, "y");

            lua_rawseti(l, tab, i);
        }

        qDebug() << "Returning lua vars: " << luaL_typename(l, lua_gettop(l)-1) << ", " <<
                luaL_typename(l, lua_gettop(l));

        return 2;
    }
    else
    {
        lua_pushboolean(l, false); // Failed to calc path
        return 1;
    }
}


void registerBindings()
{
    // Vector
    NLua::registerFunction(vecNew, "newvector", "nav");
    NLua::registerClassFunction(vecGetX, "x", "vector");
    NLua::registerClassFunction(vecSetX, "setx", "vector");
    NLua::registerClassFunction(vecGetY, "y", "vector");
    NLua::registerClassFunction(vecSetY, "sety", "vector");
    NLua::registerClassFunction(vecSet, "set", "vector");
    NLua::registerClassFunction(vecOperator, "__add", "vector", (void *)"+");
    NLua::registerClassFunction(vecOperator, "__sub", "vector", (void *)"-");
    NLua::registerClassFunction(vecOperator, "__mul", "vector", (void *)"*");
    NLua::registerClassFunction(vecOperator, "__div", "vector", (void *)"/");
    NLua::registerClassFunction(vecEqual, "__eq", "vector");
    NLua::registerClassFunction(vecLength, "__len", "vector");
    NLua::registerClassFunction(vecNormalize, "normalize", "vector");
    NLua::registerClassFunction(vecToString, "__tostring", "vector");

    // Path engine
    NLua::registerFunction(pathENew, "newpathengine", "nav");
    NLua::registerClassFunction(pathESetGrid, "setgrid", "pathengine");
    NLua::registerClassFunction(pathEExpandGrid, "expandgrid", "pathengine");
    NLua::registerClassFunction(pathESetObstacle, "setobstacle", "pathengine");
    NLua::registerClassFunction(pathEInitPath, "init", "pathengine");
    NLua::registerClassFunction(pathECalcPath, "calc", "pathengine");
}

}
