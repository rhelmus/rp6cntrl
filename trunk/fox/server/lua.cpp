#include <iostream>

#include <QCoreApplication>
#include <QStringList>
#include <QTimer>

#include "lua.h"

namespace {

// Table of libs that should be loaded
const luaL_Reg libtable[] = {
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

void luaError(lua_State *l)
{
    const char *errmsg = lua_tostring(l, -1);
    std::cerr << "Lua error: " << errmsg << "\n";
//     QCoreApplication::exit(1);
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

}


void CLuaThinker::timeOut()
{
    lua_getglobal(luaState, "think");
    if (lua_pcall(luaState, 0, 0, 0))
        luaError(luaState);
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
    {
        std::cerr << "Could not open lua VM\n";
        QCoreApplication::exit(1);
    }
    
    const luaL_Reg *lib = libtable;
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

void CLuaInterface::getClassMT(const char *type)
{
    bool init = (luaL_newmetatable(luaState, type) != 0);

    if (init)
    {
        int mt = lua_gettop(luaState);

        lua_pushstring(luaState, "__index");
        lua_pushvalue(luaState, mt);
        lua_settable(luaState, mt); // __index = metatable
    }
}

void CLuaInterface::exec()
{
    if (luaL_dofile(luaState, "main.lua"))
        luaError(luaState);

    luaThinker->start();
}

void CLuaInterface::registerFunction(lua_CFunction f, const char *n, void *d)
{
    if (d)
    {
        lua_pushlightuserdata(luaState, d);
        lua_pushcclosure(luaState, f, 1);
    }
    else
        lua_pushcfunction(luaState, f);
    lua_setglobal(luaState, n);
}

void CLuaInterface::registerClassFunction(lua_CFunction f, const char *n,
                                          const char *t, void *d)
{
    getClassMT(t);
    const int mt = lua_gettop(luaState);

    lua_pushstring(luaState, n);

    if (d)
    {
        lua_pushlightuserdata(luaState, d);
        lua_pushcclosure(luaState, f, 1);
    }
    else
        lua_pushcfunction(luaState, f);

    lua_settable(luaState, mt);
    lua_remove(luaState, mt);
}

void CLuaInterface::createClass(void *data, const char *type, lua_CFunction destr)
{
    void **p = static_cast<void **>(lua_newuserdata(luaState, sizeof(void **)));
    *p = data;
    const int ud = lua_gettop(luaState);

    getClassMT(type);
    const int mt = lua_gettop(luaState);

    lua_pushvalue(luaState, mt);
    lua_setmetatable(luaState, ud); // Set metatable for userdata

    if (destr) // Add destructor?
    {
        lua_pushstring(luaState, "__gc");
        lua_pushcfunction(luaState, destr);
        lua_settable(luaState, mt);
    }

    lua_remove(luaState, mt);

    // New userdata is left on stack
}

void CLuaInterface::runScript(const QByteArray &s)
{
    lua_getglobal(luaState, "runscript");
    lua_pushstring(luaState, s.constData());

    if (lua_pcall(luaState, 1, 0, 0))
        luaError(luaState);
}

void CLuaInterface::execScriptCmd(const QString &cmd, const QStringList &args)
{
    lua_getglobal(luaState, "execcmd");
    lua_pushstring(luaState, cmd.toLatin1().data());
    foreach(QString a, args)
    {
        lua_pushstring(luaState, a.toLatin1().data());
    }

    if (lua_pcall(luaState, 1 + args.size(), 0, 0))
        luaError(luaState);
}
