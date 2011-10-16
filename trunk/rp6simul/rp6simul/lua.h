#ifndef LUA_H
#define LUA_H

#include <lua.hpp>

#include <QMap>
#include <QMutex>
#include <QVariant>

namespace NLua
{
void stackDump(lua_State *l);
void luaError(lua_State *l, bool fatal);
void registerFunction(lua_State *l, lua_CFunction func, const char *name,
                      void *d=0);
void registerFunction(lua_State *l, lua_CFunction func, const char *name,
                      const char *tab, void *d=0);
void registerClassFunction(lua_State *l, lua_CFunction func, const char *name,
                           const char *type, void *d=NULL);
void createClass(lua_State *l, void *data, const char *type,
                 lua_CFunction destr=0, void *destrd=0);
void setVariable(lua_State *l, int val, const char *var, const char *tab);
bool checkBoolean(lua_State *l, int index);
QVariantHash convertLuaTable(lua_State *l, int index);
inline int luaAbsIndex(lua_State *l, int i)
{ return ((i < 0) && (i > LUA_REGISTRYINDEX)) ? (lua_gettop(l)+1)+i : i; }
void pushStringList(lua_State *l, const QStringList &list);
QStringList getStringList(lua_State *l, int index);

template <typename C> C *checkClassData(lua_State *l, int index, const char *type)
{
    void **p = static_cast<void **>(luaL_checkudata(l, index, type));
    return static_cast <C*>(*p);
}

template <typename C> C getFromClosure(lua_State *l, int n=1)
{
    return reinterpret_cast<C>(lua_touserdata(l, lua_upvalueindex(n)));
}


// Interface for lua mutex locking
void addLuaLockState(lua_State *l);
void clearLuaLockStates(void);

// Simple linked list: thread safety
struct SLuaLockList
{
    lua_State *luaState;
    QMutex mutex;
    SLuaLockList *next;
    SLuaLockList(void) : luaState(0), mutex(QMutex::Recursive), next(0) { }
};

extern SLuaLockList *luaLockList;

class CLuaLocker
{
    SLuaLockList *mutexListEntry;
    bool locked;

public:
    CLuaLocker(lua_State *l) : locked(false)
    {
        mutexListEntry = luaLockList;
        while (mutexListEntry && (mutexListEntry->luaState != l))
            mutexListEntry = mutexListEntry->next;
        lock();
    }
    ~CLuaLocker(void) { unlock(); }

    void lock(void)
    { if (!locked) { mutexListEntry->mutex.lock(); locked = true; } }
    void unlock(void)
    { if (locked) { mutexListEntry->mutex.unlock(); locked = false; } }
};

}

#endif // LUA_H
