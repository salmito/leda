#ifndef _LMARSHAL_H_
#define _LMARSHAL_H_

#include "lua.h"

#define MAR_TREF 1
#define MAR_TVAL 2
#define MAR_TUSR 3

#define MAR_CHR 1
#define MAR_I32 4
#define MAR_I64 8
#define MAR_PTR sizeof(void *)

#define MAR_MAGIC 0x8e
#define SEEN_IDX  3

int mar_encode(lua_State* L);
int mar_decode(lua_State* L);
int mar_clone(lua_State* L);

#endif //_LMARSHAL_H_
