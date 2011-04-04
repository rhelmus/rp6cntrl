#ifndef LUA_H
#define LUA_H

#include <lua.hpp>

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
void setVariable(int val, const char *var, const char *tab);
bool checkBoolean(int index);

template <typename T> T *getUserData(int index)
{
    return static_cast<T*>(lua_touserdata(luaInterface, index));
}

}

#endif // LUA_H
