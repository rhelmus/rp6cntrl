#include <iostream>

#include <QCoreApplication>
#include <QDebug>
#include <QStringList>
#include <QTimer>
#include <QVariant>

#include "lua.h"

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

}

namespace NLua {

CLuaInterface luaInterface;


void CLuaThinker::timeOut()
{
    lua_getglobal(luaState, "think");
    if (lua_pcall(luaState, 0, 0, 0))
    {
        luaError(luaState, false);
        execScriptCmd("abortcurrentscript");
    }
}

void CLuaThinker::start()
{
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timeOut()));
    timer->start(5);
}


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

    luaThinker = new CLuaThinker(luaState);
}

CLuaInterface::~CLuaInterface()
{   
    if (luaState)
    {
        // Debug
        qDebug("Lua stack:\n");
        stackDump(luaState);
        
        lua_close(luaState);
        luaState = NULL;
    }

    delete luaThinker;
}

void CLuaInterface::exec()
{
    if (luaL_dofile(luaState, "main.lua"))
        luaError(luaState, true);

    luaThinker->start();
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

void registerFunction(lua_CFunction func, const char *name, const char *tab, void *d)
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

void registerClassFunction(lua_CFunction func, const char *name,
                           const char *type, void *d)
{
    getClassMT(type);
    const int mt = lua_gettop(luaInterface);

    lua_pushstring(luaInterface, name);

    if (d)
    {
        lua_pushlightuserdata(luaInterface, d);
        lua_pushcclosure(luaInterface, func, 1);
    }
    else
        lua_pushcfunction(luaInterface, func);

    lua_settable(luaInterface, mt);
    lua_remove(luaInterface, mt);
}

void createClass(lua_State *l, void *data, const char *type, lua_CFunction destr)
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
        lua_pushcfunction(l, destr);
        lua_settable(l, mt);
    }

    lua_remove(l, mt);

    // New userdata is left on stack
}

void runScript(const QByteArray &s)
{
    lua_getglobal(luaInterface, "runscript");
    lua_pushstring(luaInterface, s.constData());

    if (lua_pcall(luaInterface, 1, 0, 0))
        luaError(luaInterface, false);
}

void execScriptCmd(const QString &cmd, const QStringList &args)
{
    lua_getglobal(luaInterface, "execcmd");
    lua_pushstring(luaInterface, cmd.toLatin1().data());
    foreach(QString a, args)
    {
        lua_pushstring(luaInterface, a.toLatin1().data());
    }

    if (lua_pcall(luaInterface, 1 + args.size(), 0, 0))
        luaError(luaInterface, false);
}

void scriptInitClient()
{
    lua_getglobal(luaInterface, "initclient");
    if (lua_pcall(luaInterface, 0, 0, 0))
        luaError(luaInterface, false);
}

QMap<QString, QVariant> convertLuaTable(lua_State *l, int index)
{
    QMap<QString, QVariant> ret;
    const int tabind = luaAbsIndex(l, index);

    stackDump(l);

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

}
