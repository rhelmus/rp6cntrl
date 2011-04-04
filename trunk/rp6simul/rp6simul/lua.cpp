#include "lua.h"

#include <QDebug>
#include <QtGlobal>

namespace {

// Table of libs that should be loaded
const luaL_Reg libTable[] = {
    { "", luaopen_base },
    { LUA_TABLIBNAME, luaopen_table },
    { LUA_IOLIBNAME, luaopen_io },
    { LUA_OSLIBNAME, luaopen_os },
    { LUA_STRLIBNAME, luaopen_string },
    { LUA_MATHLIBNAME, luaopen_math },
    { LUA_LOADLIBNAME, luaopen_package },
    { LUA_DBLIBNAME, luaopen_debug },
    { NULL, NULL }
};

void luaError(lua_State *l, bool fatal)
{
    const char *errmsg = lua_tostring(l, -1);
    if (!fatal)
        qCritical() << "Lua error: " << errmsg << "\n";
    else
        qFatal("Lua error: %s\n", errmsg);
}

// Set global variable at stack
void setGlobalVar(const char *var, const char *tab)
{
    if (!tab)
        lua_setglobal(NLua::luaInterface, var);
    else
    {
        lua_getglobal(NLua::luaInterface, tab);

        if (lua_isnil(NLua::luaInterface, -1))
        {
            lua_pop(NLua::luaInterface, 1);
            lua_newtable(NLua::luaInterface);
        }

        lua_insert(NLua::luaInterface, -2); // Swap table <--> value
        lua_setfield(NLua::luaInterface, -2, var);

        lua_setglobal(NLua::luaInterface, tab);
    }
}

}

namespace NLua {

CLuaInterface luaInterface;

CLuaInterface::CLuaInterface()
{
    // Initialize lua
    luaState = lua_open();

    if (!luaState)
        qFatal("Could not open lua VM\n");

    const luaL_Reg *lib = libTable;
    for (; lib->func; lib++)
    {
        lua_pushcfunction(luaState, lib->func);
        lua_pushstring(luaState, lib->name);
        lua_call(luaState, 1, 0);
        lua_settop(luaState, 0);  // Clear stack
    }
}

CLuaInterface::~CLuaInterface()
{
    if (luaState)
    {
        // Debug
        qDebug("Lua stack:\n");
        stackDump(luaState);

        lua_close(luaState);
        luaState = 0;
    }
}

void CLuaInterface::exec()
{
    if (luaL_dofile(luaState, "lua/main.lua"))
        luaError(luaState, true);
}


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

void registerFunction(lua_CFunction func, const char *name, void *d)
{
    if (d)
    {
        lua_pushlightuserdata(luaInterface, d);
        lua_pushcclosure(luaInterface, func, 1);
    }
    else
        lua_pushcfunction(luaInterface, func);
    lua_setglobal(luaInterface, name);
}

void registerFunction(lua_CFunction func, const char *name, const char *tab,
                      void *d)
{
    lua_getglobal(luaInterface, tab);

    if (lua_isnil(luaInterface, -1))
    {
        lua_pop(luaInterface, 1);
        lua_newtable(luaInterface);
    }

    if (d)
    {
        lua_pushlightuserdata(luaInterface, d);
        lua_pushcclosure(luaInterface, func, 1);
    }
    else
        lua_pushcfunction(luaInterface, func);

    lua_setfield(luaInterface, -2, name);
    lua_setglobal(luaInterface, tab);
}

void setVariable(int val, const char *var, const char *tab)
{
    lua_pushinteger(luaInterface, val);
    setGlobalVar(var, tab);
}

bool checkBoolean(int index)
{
    luaL_checktype(luaInterface, index, LUA_TBOOLEAN);
    return lua_toboolean(luaInterface, index);
}

}
