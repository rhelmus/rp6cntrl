#include "lua.h"
#include "utils.h"

#include <QDebug>
#include <QDir>
#include <QtGlobal>
#include <QStringList>

namespace {

// Set global variable at stack
void setGlobalVar(lua_State *l, const char *var, const char *tab)
{
    if (!tab)
        lua_setglobal(l, var);
    else
    {
        lua_getglobal(l, tab);

        if (lua_isnil(l, -1))
        {
            lua_pop(l, 1);
            lua_newtable(l);
        }

        lua_insert(l, -2); // Swap table <--> value
        lua_setfield(l, -2, var);

        lua_setglobal(l, tab);
    }
}

void getClassMT(lua_State *l, const char *type)
{
    bool init = (luaL_newmetatable(l, type) != 0);

    if (init)
    {
        int mt = lua_gettop(l);

        lua_pushstring(l, "__index");
        lua_pushvalue(l, mt);
        lua_settable(l, mt); // __index = metatable
    }

    // MT left on stack
}

}


namespace NLua {

// Based from a example in the book "Programming in Lua"
void stackDump(lua_State *l)
{
    int top = lua_gettop(l);
    for (int i = 1; i <= top; i++)
    {
        /* repeat for each level */
        int t = lua_type(l, i);
        switch (t)
        {
            case LUA_TSTRING:  /* strings */
                qDebug("`%s'\n", lua_tostring(l, i));
                break;
            case LUA_TBOOLEAN:  /* booleans */
                qDebug("%s\n", lua_toboolean(l, i) ? "true" : "false");
                break;
            case LUA_TNUMBER:  /* numbers */
                qDebug("%g\n", lua_tonumber(l, i));
                break;
            default:  /* other values */
                qDebug("%s\n", lua_typename(l, t));
                break;
        }
    }
    qDebug("---------------\n");  /* end the listing */
}

void registerFunction(lua_State *l, lua_CFunction func, const char *name,
                      void *d)
{
    if (d)
    {
        lua_pushlightuserdata(l, d);
        lua_pushcclosure(l, func, 1);
    }
    else
        lua_pushcfunction(l, func);
    lua_setglobal(l, name);
}

void registerFunction(lua_State *l, lua_CFunction func, const char *name,
                      const char *tab, void *d)
{
    lua_getglobal(l, tab);

    if (lua_isnil(l, -1))
    {
        lua_pop(l, 1);
        lua_newtable(l);
    }

    if (d)
    {
        lua_pushlightuserdata(l, d);
        lua_pushcclosure(l, func, 1);
    }
    else
        lua_pushcfunction(l, func);

    lua_setfield(l, -2, name);
    lua_setglobal(l, tab);
}

void registerClassFunction(lua_State *l, lua_CFunction func, const char *name,
                           const char *type, void *d)
{
    getClassMT(l, type);
    const int mt = lua_gettop(l);

    lua_pushstring(l, name);

    if (d)
    {
        lua_pushlightuserdata(l, d);
        lua_pushcclosure(l, func, 1);
    }
    else
        lua_pushcfunction(l, func);

    lua_settable(l, mt);
    lua_remove(l, mt);
}

void createClass(lua_State *l, void *data, const char *type, lua_CFunction destr,
                 void *destrd)
{
    void **p = static_cast<void **>(lua_newuserdata(l, sizeof(void **)));
    *p = data;
    const int ud = lua_gettop(l);

    getClassMT(l, type);
    const int mt = lua_gettop(l);

    lua_pushvalue(l, mt);
    lua_setmetatable(l, ud); // Set metatable for userdata

    if (destr) // Add destructor?
    {
        lua_pushstring(l, "__gc");

        if (destrd) // Destructor private data?
        {
            lua_pushlightuserdata(l, destrd);
            lua_pushcclosure(l, destr, 1);
        }
        else
            lua_pushcfunction(l, destr);

        lua_settable(l, mt);
    }

    lua_remove(l, mt);

    // New userdata is left on stack
}

void setVariable(lua_State *l, int val, const char *var, const char *tab)
{
    lua_pushinteger(l, val);
    setGlobalVar(l, var, tab);
}

bool checkBoolean(lua_State *l, int index)
{
    luaL_checktype(l, index, LUA_TBOOLEAN);
    return lua_toboolean(l, index);
}

QVariantHash convertLuaTable(lua_State *l, int index)
{
    QVariantHash ret;
    const int tabind = luaAbsIndex(l, index);

    lua_pushnil(l);
    while (lua_next(l, tabind))
    {
        QVariant v;
        switch (lua_type(l, -1))
        {
        case LUA_TBOOLEAN: v.setValue(static_cast<bool>(lua_toboolean(l, -1))); break;
        case LUA_TNUMBER: v.setValue(static_cast<float>(lua_tonumber(l, -1))); break;
        default:
        case LUA_TSTRING: v.setValue(QString(lua_tostring(l, -1))); break;
        case LUA_TTABLE: v.setValue(convertLuaTable(l, -1)); break;
        }

        // According to manual we cannot use tostring directly on keys.
        // Therefore duplicate the key first
        lua_pushvalue(l, -2);
        ret[lua_tostring(l, -1)] = v;

        lua_pop(l, 2); // Remove duplicate key and value, keep original key
    }

    return ret;
}

void pushStringList(lua_State *l, const QStringList &list)
{
    lua_newtable(l);
    int i = 1;
    foreach(QString s, list)
    {
        lua_pushstring(l, qPrintable(s));
        lua_rawseti(l, -2, i);
        ++i;
    }

    // New table now on lua stack
}

QStringList getStringList(lua_State *l, int index)
{
    const int tabind = luaAbsIndex(l, index);
    const int size = lua_objlen(l, tabind);
    QStringList ret;

    for (int i=1; i<=size; ++i)
    {
        lua_rawgeti(l, tabind, i);
        ret << luaL_checkstring(l, -1);
        lua_pop(l, 1); // Pop value
    }

    lua_pop(l, 1); // Pop table
    return ret;
}


// Lua mutex lock interface

SLuaLockList *luaLockList = 0;

void addLuaLockState(lua_State *l)
{
    // NOT thread safe
    SLuaLockList *entry = new SLuaLockList;
    entry->luaState = l;
    entry->next = 0;

    if (!luaLockList)
    {
        luaLockList = entry;
        return;
    }

    SLuaLockList *it = luaLockList;
    while (it)
    {
        if (!it->next)
        {
            it->next = entry;
            break;
        }
        it = it->next;
    }
}

void clearLuaLockStates()
{
    SLuaLockList *it = luaLockList;
    while (it)
    {
        SLuaLockList *it2 = it;
        it = it->next;
        delete it2;
    }
    luaLockList = 0;
}


}
