#ifndef LUA_H
#define LUA_H

#include <QByteArray>
#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>

#include <lua.hpp>

namespace NLua
{

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

public:
    CLuaInterface(void);
    ~CLuaInterface(void);

    void exec(void);

    operator lua_State*(void) { return luaState; }
};

extern CLuaInterface luaInterface;

void stackDump(lua_State *l);
void getClassMT(lua_State *l, const char *type);
inline void getClassMT(const char *type) { getClassMT(luaInterface, type); }
void registerFunction(lua_CFunction func, const char *name, void *d=NULL);
void registerFunction(lua_CFunction func, const char *name, const char *tab, void *d=NULL);
void registerClassFunction(lua_CFunction func, const char *name, const char *type,
                           void *d=NULL);
void createClass(lua_State *l, void *data, const char *type, lua_CFunction destr = NULL);
void runScript(const QByteArray &s);
void execScriptCmd(const QString &cmd, const QStringList &args=QStringList());
void scriptInitClient(void);

inline int luaAbsIndex(lua_State *l, int i)
{ return ((i < 0) && (i > LUA_REGISTRYINDEX)) ? (lua_gettop(l)+1)+i : i; }

template <typename C> C *checkClassData(lua_State *l, int index, const char *type)
{
    void **p = static_cast<void **>(luaL_checkudata(l, index, type));
    return static_cast <C*>(*p);
}

QMap<QString, QVariant> convertLuaTable(lua_State *l, int index);

}

#endif
