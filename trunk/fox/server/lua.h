#include <lua.hpp>

class CLuaInterface
{
    lua_State *luaState;

public:
    CLuaInterface(void);
    ~CLuaInterface(void);

    void exec(void);
    void registerFunction(lua_CFunction f, const char *n, void *d=NULL);
    void runScript(const char *s);

    operator lua_State*(void) { return luaState; }
};