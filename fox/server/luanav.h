#ifndef LUANAV_H
#define LUANAV_H

#include "lua.h"

namespace NLuaNav
{

// Lua vector bindings
int vecNew(lua_State *l);
int vecDel(lua_State *l);
int vecGetX(lua_State *l);
int vecSetX(lua_State *l);
int vecGetY(lua_State *l);
int vecSetY(lua_State *l);
int vecSet(lua_State *l);
int vecOperator(lua_State *l);
int vecEqual(lua_State *l);
int vecLength(lua_State *l);
int vecNormalize(lua_State *l);
int vecToString(lua_State *l);

// Bindings for Lua pathengine class
int pathENew(lua_State *l);
int pathEDel(lua_State *l);
int pathESetGrid(lua_State *l);
int pathExpandGrid(lua_State *l);
int pathESetObstacle(lua_State *l);
int pathEInitPath(lua_State *l);
int pathECalcPath(lua_State *l);

void registerBindings(void);

}

#endif // LUANAV_H
