/* ======================================================================
** memarray.c - Copyright (C) 2005-2006 Varol Kaptan
** see LICENSE for more information
** ======================================================================
**/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

#define MYNAME "memarray"
#define MYTYPE "memarray*"
#define VERSION "0.5"

#define MEMARRAY_TCHAR        0
#define MEMARRAY_TUCHAR       1
#define MEMARRAY_TSHORT       2
#define MEMARRAY_TUSHORT      3
#define MEMARRAY_TINT         4
#define MEMARRAY_TUINT        5
#define MEMARRAY_TLONG        6
#define MEMARRAY_TULONG       7
#define MEMARRAY_TFLOAT       8
#define MEMARRAY_TDOUBLE      9
#define MEMARRAY_NUM_TYPES    10

#define MEMARRAY_TYPE_MASK 0x1F
#define MEMARRAY_INHERITED 0x20

#define mem_type(m)        ((m)->flags & MEMARRAY_TYPE_MASK)
#define mem_inherited(m)   ((m)->flags & MEMARRAY_INHERITED)

#if LUA_VERSION_NUM > 501
	#define luaL_putchar luaL_addchar
	#define luaL_openlib(L, n, r, p) luaL_setfuncs (L,r,0)
#endif

typedef struct {
   char *name;
   int   size;
} typeinfo;

const typeinfo m_types[MEMARRAY_NUM_TYPES] = {
   { "char",      sizeof(char)             },
   { "uchar",     sizeof(unsigned char)    },
   { "short",     sizeof(short)            },
   { "ushort",    sizeof(unsigned short)   },
   { "int",       sizeof(int)              },
   { "uint",      sizeof(unsigned int)     },
   { "long",      sizeof(long)             },
   { "ulong",     sizeof(unsigned long)    },
   { "float",     sizeof(float)            },
   { "double",    sizeof(double)           },
};

typedef struct memarray {
   size_t   size;
   size_t   length;
   unsigned int flags;
   void     *data;
} memarray_t;

/* we keep discovered endianness here (1 = little endian, 0 = big endian) */
static int little_endian = 0;

static int get_memtype(const char * typename)
{
   int type;

   for (type = 0; type < MEMARRAY_NUM_TYPES; ++type) {
      if (strcmp(typename, m_types[type].name) == 0)
         return type;
   }

   return -1;
}

static const void * check_lightuserdata(lua_State *L, int n)
{
   if (lua_type(L,n) != LUA_TLIGHTUSERDATA)
      luaL_error(L, "lightuserdata expected");
   return (const void *) lua_touserdata(L, n);
}

static memarray_t *memarray_get(lua_State * L, int i)
{
   if (luaL_checkudata(L, i, MYTYPE) == NULL)
      luaL_error(L,"Memarray userdata expected");
   memarray_t *m=lua_touserdata(L, i);
   if (!mem_inherited(m) && m->data==NULL){
		luaL_error(L,"Data was invalidated");
	}
   return m;
}

static memarray_t *memarray_new(lua_State * L)
{
   memarray_t *m = lua_newuserdata(L, sizeof(memarray_t));

   m->data = NULL;
   m->size = 0;
   m->length = 0;
   m->flags = MEMARRAY_INHERITED;

   luaL_getmetatable(L, MYTYPE);
   lua_setmetatable(L, -2);
   /* fprintf(stderr, "memarray_new @ %p\n", (void *)m); */
   return m;
}

static void memarray_delete(memarray_t *m)
{
   if (!mem_inherited(m) && m->data){
	      free(m->data);
	     	/*printf("Collect %p\n",m);*/
	}
   m->data = NULL;
   m->size = 0;
   m->length = 0;
   m->flags = MEMARRAY_INHERITED;
}

/* self, 'type', n, [address] */
static int Lrealloc(lua_State * L)
{
   memarray_t *m;
   const char *typename;
   int type;
   size_t length;
   size_t size;
   void * data;
   char errmsg[128];

   m = memarray_get(L, 1);
   typename = luaL_optstring(L, 2, "uchar");

   if ( (type = get_memtype(typename)) < 0 ) {
      luaL_error(L, "memarray:realloc(): unknown type '%s'", typename);
      return 0;
   }

   length  = (size_t) luaL_optnumber(L, 3, 0);
   size = length * m_types[type].size;

   if (lua_gettop(L) > 3) {
      /* do not allocate memory, inherit something else */
      memarray_delete(m);
      m->size    = size;
      m->length  = length;
      m->flags   = type | MEMARRAY_INHERITED;
      m->data    = lua_touserdata(L, 4);
   } else {
      /* allocate memory */
      memarray_delete(m);
      data = (void *) malloc(size);
      if ((size != 0) && (data == NULL)) {
         m->flags = type | MEMARRAY_INHERITED;
         sprintf(errmsg, "memarray:__call(): memory allocation failure "
            "for %zd bytes (%s x %zd)\n", size, typename, length);
         luaL_error(L, errmsg);
      } else {
         m->flags   = type;
         m->size    = size;
         m->length  = length;
         m->data    = data;
      }
   }
   lua_settop(L, 1);
   return 1;
}

/* memarray, 'type', n, [address] */
static int L__call(lua_State * L)
{
   memarray_new(L);
   lua_replace(L, 1);
   return Lrealloc(L);
}

/* 'type' */
static int Lsizeof(lua_State * L)
{
   const char *typename;
   int type;

   typename = luaL_checkstring(L, 1);

   if ( (type = get_memtype(typename)) < 0 ) {
      luaL_error(L, "memarray:sizeof(): unknown type '%s'", typename);
      return 0;
   }

   lua_pushnumber(L, m_types[type].size);

   return 1;
}

/* dst, src, size(in bytes) */
static int Lmemcpy(lua_State * L)
{
   void * dst = (void *) check_lightuserdata(L, 1);
   const void * src = check_lightuserdata(L, 2);
   size_t size = (size_t) luaL_checknumber(L, 3);
   memcpy(dst, src, size);
   return 1;
}

/* self, index */
static int L__index(lua_State * L)
{
   memarray_t *m;
   size_t index;

   m = memarray_get(L, 1);

   switch (lua_type(L, 2)) {
      case LUA_TNUMBER:
         index = (size_t) lua_tonumber(L, 2);
         if ( /*(index < 0) ||*/ (index >= m->length) ) {
            lua_pushnil(L);
            luaL_error(L, "index out of range");
         } else switch (mem_type(m)) {
            case MEMARRAY_TCHAR:   lua_pushnumber(L, ((char *)(m->data))[index]); break;
            case MEMARRAY_TUCHAR:  lua_pushnumber(L, ((unsigned char *)(m->data))[index]); break;
            case MEMARRAY_TSHORT:  lua_pushnumber(L, ((short *)(m->data))[index]); break;
            case MEMARRAY_TUSHORT: lua_pushnumber(L, ((unsigned short *)(m->data))[index]); break;
            case MEMARRAY_TINT:    lua_pushnumber(L, ((int *)(m->data))[index]); break;
            case MEMARRAY_TUINT:   lua_pushnumber(L, ((unsigned int *)(m->data))[index]); break;
            case MEMARRAY_TLONG:   lua_pushnumber(L, ((long *)(m->data))[index]); break;
            case MEMARRAY_TULONG:  lua_pushnumber(L, ((unsigned long *)(m->data))[index]); break;
            case MEMARRAY_TFLOAT:  lua_pushnumber(L, ((float *)(m->data))[index]); break;
            case MEMARRAY_TDOUBLE: lua_pushnumber(L, ((double *)(m->data))[index]); break;
            default: lua_pushnil(L); break;
         }
         break;

      default:
         lua_getmetatable(L, 1);
         lua_replace(L, 1);
         lua_rawget(L, 1);
         break;
   }
   return 1;
}

/* self, index, value */
static int L__newindex(lua_State * L)
{
   memarray_t *m;
   size_t index;
   lua_Number value;

   m = memarray_get(L, 1);

   switch (lua_type(L, 2)) {
      case LUA_TNUMBER:
         index = (size_t) lua_tonumber(L, 2);
         value = lua_tonumber(L, 3);
         if ( /*(index < 0) ||*/ (index >= m->length) ) {
            luaL_error(L, "index out of range");
         } else switch (mem_type(m)) {
            case MEMARRAY_TCHAR:   ((char *)m->data)[index]           = (char) value; break;
            case MEMARRAY_TUCHAR:  ((unsigned char *)m->data)[index]  = (unsigned char) value; break;
            case MEMARRAY_TSHORT:  ((short *)m->data)[index]          = (short) value; break;
            case MEMARRAY_TUSHORT: ((unsigned short *)m->data)[index] = (unsigned short) value; break;
            case MEMARRAY_TINT:    ((int *)m->data)[index]            = (int) value; break;
            case MEMARRAY_TUINT:   ((unsigned int *)m->data)[index]   = (unsigned int) value; break;
            case MEMARRAY_TLONG:   ((long *)m->data)[index]           = (long) value; break;
            case MEMARRAY_TULONG:  ((unsigned long *)m->data)[index]  = (unsigned long) value; break;
            case MEMARRAY_TFLOAT:  ((float *)m->data)[index]          = (float) value; break;
            case MEMARRAY_TDOUBLE: ((double *)m->data)[index]         = (double) value; break;
            default: lua_pushnil(L); break;
         }
         break;

      default:
         luaL_error(L, "memarray*:__newindex key must be number");
         break;
   }
   return 0;
}

/* self */
static int L__tostring(lua_State * L)
{
   memarray_t *m = memarray_get(L, 1);
   char s[256];

   sprintf(s,
      mem_inherited(m) ? "memarray(%s,%zd,%p,inherited)@%p": "memarray(%s,%zd,%p)@%p" ,
      m_types[mem_type(m)].name, m->length, (void *)m->data, (void *)m);

   lua_pushstring(L, s);
   return 1;
}

/* self */
static int Lsize(lua_State *L)
{
   memarray_t *m = memarray_get(L, 1);
   lua_pushnumber(L, m->size);
   return 1;
}

/* self */
static int Llength(lua_State *L)
{
   memarray_t *m = memarray_get(L, 1);
   lua_pushnumber(L, m->length);
   return 1;
}

/* self, [index] */
static int Lptr(lua_State *L)
{
   memarray_t *m = memarray_get(L, 1);
   size_t index = (size_t) luaL_optnumber(L, 2, 0);

   if ( /*(index < 0) ||*/ (index >= m->length) ) {
      luaL_error(L, "index out of range");
      return 0;
   } else {
      char *ptr = m->data;
      lua_pushlightuserdata(L, (void *)(ptr + index * m_types[mem_type(m)].size));
      return 1;
   }
}

static int Lto_ptr(lua_State *L) {
   memarray_t *m = memarray_get(L, 1);

   char *ptr = m->data;
   lua_pushlightuserdata(L, (void *)ptr);
   m->data=NULL;
   return 1;
}

static int Lfrom_ptr(lua_State *L) {
   const char *typename;
   int type;
   size_t length;
   size_t size;
   void * data;

   typename = lua_tostring(L, 2);

   if ( (type = get_memtype(typename)) < 0 ) {
      luaL_error(L, "memarray:realloc(): unknown type '%s'", typename);
      return 0;
   }

   data=(void *)check_lightuserdata(L,1);
   memarray_t *m=(memarray_t *)memarray_new(L);
   lua_replace(L,1);
   length  = (size_t) lua_tonumber(L, 3);
   size = length * m_types[type].size;

   m->flags   = type;
   m->size    = size;
   m->length  = length;
   m->data    = data;
   lua_settop(L,1);
   return 1;
}



/* self */
static int L__gc(lua_State *L)
{
   if (luaL_checkudata(L, 1, MYTYPE) == NULL)
      luaL_error(L,"Memarray userdata expected");
   memarray_t *m=lua_touserdata(L, 1);
   /*
   fprintf(stderr, "invoking __gc for ");
   fprintf(stderr,
      mem_inherited(m) ? "memarray(%s,%zd,%p,inherited)@%p\n": "memarray(%s,%zd,%p)@%p\n" ,
      m_types[mem_type(m)].name, m->length, (void *)m->data, (void *)m);
   */
   memarray_delete(m);
   return 0;
}

/* self */
static int Lto_str(lua_State *L)
{
   memarray_t *m = memarray_get(L, 1);
   luaL_Buffer b;

   luaL_buffinit(L, &b);
   luaL_addlstring(&b, m->data, m->size);
   luaL_pushresult(&b);

   return 1;
}

/* self, str */
static int Lfrom_str(lua_State *L)
{
   size_t len;
   memarray_t *m = memarray_get(L, 1);
   const char *str = luaL_checklstring(L, 2, &len);

   if (m->size != len) {
      luaL_error(L, "memarray:to_str size mismatch");
      return 0;
   } else {
      memcpy(m->data, str, len);
   }
   lua_settop(L,1);
   return 1;
}

#define PUT_SWAP_2(B, src) \
   luaL_putchar((B), (src)[1]); \
   luaL_putchar((B), (src)[0]);

#define PUT_SWAP_4(B, src) \
   luaL_putchar((B), (src)[3]); \
   luaL_putchar((B), (src)[2]); \
   luaL_putchar((B), (src)[1]); \
   luaL_putchar((B), (src)[0]);

#define PUT_SWAP_8(B, src) \
   luaL_putchar((B), (src)[7]); \
   luaL_putchar((B), (src)[6]); \
   luaL_putchar((B), (src)[5]); \
   luaL_putchar((B), (src)[4]); \
   luaL_putchar((B), (src)[3]); \
   luaL_putchar((B), (src)[2]); \
   luaL_putchar((B), (src)[1]); \
   luaL_putchar((B), (src)[0]);

/* self */
static int Lto_netstr(lua_State *L)
{
   memarray_t *m = memarray_get(L, 1);
   luaL_Buffer b;
   size_t item_size;
   unsigned char *src, *end;

   luaL_buffinit(L, &b);
   item_size = m->size / m->length;

   if (little_endian && (item_size != 1)) {
      src = (unsigned char *) m->data;
      end = src + m->size;
      switch (item_size) {
         case 2:
            while (src < end) { PUT_SWAP_2(&b, src); src += 2; }
            break;

         case 4:
            while (src < end) { PUT_SWAP_4(&b, src); src += 4; }
            break;

         case 8:
            while (src < end) { PUT_SWAP_8(&b, src); src += 8; }
            break;

         default:
            luaL_error(L, "memarray:to_netstr(): unsupported item_size");
            break;
      }
   } else {
      luaL_addlstring(&b, m->data, m->size);
   }

   luaL_pushresult(&b);

   return 1;
}

#define COPY_SWAP_2(src,dst) \
   dst[0] = src[1]; \
   dst[1] = src[0];

#define COPY_SWAP_4(src, dst) \
   dst[0] = src[3]; \
   dst[1] = src[2]; \
   dst[2] = src[1]; \
   dst[3] = src[0];

#define COPY_SWAP_8(src, dst) \
   dst[0] = src[7]; \
   dst[1] = src[6]; \
   dst[2] = src[5]; \
   dst[3] = src[4]; \
   dst[4] = src[3]; \
   dst[5] = src[2]; \
   dst[6] = src[1]; \
   dst[7] = src[0];

/* self, str */
static int Lfrom_netstr(lua_State *L)
{
   size_t len;
   memarray_t *m = memarray_get(L, 1);
   const char *str = luaL_checklstring(L, 2, &len);
   size_t item_size;
   unsigned char *src, *end, *dst;

   if (m->size != len) {
      luaL_error(L, "memarray:to_netstr size mismatch");
      return 0;
   } else {
      item_size = m->size / m->length;

      if (little_endian && (item_size != 1)) {
         src = (unsigned char *) str;
         end = src + m->size;
         dst = (unsigned char *) m->data;
         switch (item_size) {
            case 2:
               while (src < end) { COPY_SWAP_2(src, dst); src += 2; dst += 2; }
               break;

            case 4:
               while (src < end) { COPY_SWAP_4(src, dst); src += 4; dst += 4; }
               break;

            case 8:
               while (src < end) { COPY_SWAP_8(src, dst); src += 8; dst += 8; }
               break;

            default:
               luaL_error(L, "memarray:from_netstr(): unsupported item_size");
               break;
         }
      } else {
         memcpy(m->data, str, len);
      }
   }
   return 0;
}

static int Ltype(lua_State *L) {
   memarray_t *m = memarray_get(L, 1);
   lua_pushstring(L,m_types[mem_type(m)].name);
   return 1;
}

static const luaL_Reg reg_memarray[] =
{
   { "__call",    L__call  },
   { "sizeof",    Lsizeof  },
   { "memcpy",    Lmemcpy  },
   { "from_ptr",     Lfrom_ptr   },
   { NULL,        NULL     }
};

static const luaL_Reg reg_memarray_ptr[] =
{
   { "__index",      L__index    },
   { "__newindex",   L__newindex },
   { "__tostring",   L__tostring },
   { "__gc",         L__gc       },
   { "size",         Lsize       },
   { "length",       Llength     },
   { "ptr",          Lptr        },
   { "to_ptr",       Lto_ptr     },
   { "to_str",       Lto_str     },
   { "from_str",     Lfrom_str   },
   { "to_netstr",    Lto_netstr  },
   { "type",         Ltype       },
   { "from_netstr",  Lfrom_netstr},
   { "realloc",      Lrealloc    },
   { NULL,           NULL        }
};

LUALIB_API int luaopen_lmemarray(lua_State * L)
{
   /* we discover endianness here */
   int one = 1;
   little_endian = (*(char *)&one == 1);

   luaL_newmetatable(L, MYTYPE);
   luaL_openlib(L, NULL, reg_memarray_ptr, 0);

   lua_newtable(L);
   lua_pushvalue(L, -1);
   luaL_openlib(L, NULL, reg_memarray, 0);

   lua_setmetatable(L, -2);
   return 1;
}
