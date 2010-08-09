#include <QByteArray>
#include <QObject>
#include <QString>

#include <lua.hpp>

class CLuaThinker: public QObject
{
    Q_OBJECT

    lua_State *luaState;

private slots:
    void timeOut(void);

public:
    CLuaThinker(lua_State *l) : luaState(l) { }
    void start(void);
};

class CLuaInterface
{
    lua_State *luaState;
    CLuaThinker *luaThinker;

    void getClassMT(const char *type);

public:
    CLuaInterface(void);
    ~CLuaInterface(void);

    void exec(void);
    void registerFunction(lua_CFunction f, const char *n, void *d=NULL);
    void registerClassFunction(lua_CFunction f, const char *n, const char *t,
                               void *d=NULL);
    void createClass(void *data, const char *type, lua_CFunction destr = NULL);
    void runScript(const QByteArray &s);
    void execScriptCmd(const QString &cmd, const QStringList &args);

    operator lua_State*(void) { return luaState; }
};

template <typename C> C *checkClassData(lua_State *l, int index, const char *type)
{
    void **p = static_cast<void **>(luaL_checkudata(l, index, type));
    return static_cast <C*>(*p);
}
