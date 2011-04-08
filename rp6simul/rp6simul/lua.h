#ifndef LUA_H
#define LUA_H

#include <lua.hpp>

#include <QMutex>

namespace NLua
{

class CLuaInterface
{
    lua_State *luaState;

public:
    CLuaInterface(void);
    ~CLuaInterface(void);

    void exec(void);

    operator lua_State*(void) { return luaState; }
};

extern CLuaInterface luaInterface;

void stackDump(lua_State *l);
void registerFunction(lua_CFunction func, const char *name, void *d=0);
void registerFunction(lua_CFunction func, const char *name, const char *tab,
                      void *d=0);
void registerClassFunction(lua_CFunction func, const char *name, const char *type,
                           void *d=NULL);
void createClass(lua_State *l, void *data, const char *type, lua_CFunction destr = NULL);
void setVariable(int val, const char *var, const char *tab);
bool checkBoolean(lua_State *l, int index);

template <typename C> C *checkClassData(lua_State *l, int index, const char *type)
{
    void **p = static_cast<void **>(luaL_checkudata(l, index, type));
    return static_cast <C*>(*p);
}

class CLuaLocker
{
    static QMutex mutex;
    bool locked;

public:
    CLuaLocker(void) : locked(false) { lock(); }
    ~CLuaLocker(void) { unlock(); }

    void lock(void) { if (!locked) { mutex.lock(); locked = true; } }
    void unlock(void) { if (locked) { mutex.unlock(); locked = false; } }
};

}

#endif // LUA_H
