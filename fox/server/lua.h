#include <lua.hpp>

class CLuaInterface
{
    lua_State *luaState;

public:
    CLuaInterface(void);
    ~CLuaInterface(void);

    void exec(void);
    void registerFunction(lua_CFunction f, const char *n, void *d=NULL);
    void runScript(const QByteArray &s);
    void execScriptCmd(const QString &cmd, const QStringList &args);

    operator lua_State*(void) { return luaState; }
};