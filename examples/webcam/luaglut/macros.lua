-- ======================================================================
-- macros.lua - Copyright (C) 2005-2006 Varol Kaptan
-- see LICENSE for more information
-- ======================================================================
-- vim: set ts=3 et:

--[[

   Function signatures are encoded as a combination of letters and
   numbers. The meaning of the letters is as follows:

      OpenGL Datatypes
      ----------------
      B  boolean
      Y  GLbyte
      uY GLubyte
      H  GLshort
      uH GLushort
      I  GLint
      uI GLuint
      Z  GLsizei
      E  GLenum
      T  GLbitfield
      F  GLfloat, GLclampf
      D  GLdouble, GLclampd

      C Datatypes
      -----------
      i  int
      b  boolean
      f  float
      d  double
      s  C style string (zero terminated)
      p  a generic pointer (void *)
      v  void

   Numbers following a letter sequence mean that there are N parameters
   of that particular type.

--]]

local ptypes = {
   -- OpenGL Datatypes
   B  = { type = 'GLboolean',    getf = 'check_GLboolean',  putf = 'lua_pushnumber' },
   Y  = { type = 'GLbyte',       getf = 'check_GLbyte',     putf = 'lua_pushnumber' },
   uY = { type = 'GLubyte',      getf = 'check_GLubyte',    putf = 'lua_pushnumber' },
   H  = { type = 'GLshort',      getf = 'check_GLshort',    putf = 'lua_pushnumber' },
   uH = { type = 'GLushort',     getf = 'check_GLushort',   putf = 'lua_pushnumber' },
   I  = { type = 'GLint',        getf = 'check_GLint',      putf = 'lua_pushnumber' },
   uI = { type = 'GLuint',       getf = 'check_GLuint',     putf = 'lua_pushnumber' },
   Z  = { type = 'GLsizei',      getf = 'check_GLsizei',    putf = 'lua_pushnumber' },
   E  = { type = 'GLenum',       getf = 'check_GLenum',     putf = 'lua_pushnumber' },
   T  = { type = 'GLbitfield',   getf = 'check_GLbitfield', putf = 'lua_pushnumber' },
   F  = { type = 'GLfloat',      getf = 'check_GLfloat',    putf = 'lua_pushnumber' },
   D  = { type = 'GLdouble',     getf = 'check_GLdouble',   putf = 'lua_pushnumber' },
   -- C Datatypes
   b  = { type = 'int',          getf = 'check_int',        putf = 'lua_pushboolean'},
   i  = { type = 'int',          getf = 'check_int',        putf = 'lua_pushnumber' },
   ui = { type = 'unsigned int', getf = 'check_uint',       putf = 'lua_pushnumber' },
   f  = { type = 'float',        getf = 'check_float',      putf = 'lua_pushnumber' },
   d  = { type = 'double',       getf = 'check_double',     putf = 'lua_pushnumber' },
   s  = { type = 'const char *', getf = 'check_string',     putf = 'lua_pushstring' },
   p  = { type = 'void *',       getf = 'check_lightuserdata',  putf = 'lua_pushlightuserdata' },
   v  = { type = 'void',         getf = nil,                putf = nil              },
}

local opengl_numeric_types = { 'B', 'Y', 'uY', 'H', 'uH', 'I', 'uI', 'Z', 'E', 'T', 'F', 'D' }
local c_numeric_types = { 'i', 'ui', 'f', 'd' }

if _VERSION == 'Lua 5.1' then
   -- here is the Lua 5.1 version
   printf = loadstring('return function(...) io.write(string.format(...)) end')()
else
   -- and here is the Lua 5.0.2 version
   printf = loadstring('return function(...) io.write(string.format(unpack(arg))) end')()
end

function gen_macro(sig)

   local args = {}
   local i, imax = 1, string.len(sig)
   local nxt, old_nxt
   local prefix = ''

   while i <= imax do
      nxt, old_nxt = string.sub(sig, i, i), nxt
      if nxt == 'u' then
         prefix = nxt
      elseif tonumber(nxt) == nil then
         table.insert(args, prefix .. nxt)
         prefix = ''
      else
         for j = 1,tonumber(nxt)-1 do
            table.insert(args, prefix .. old_nxt)
         end
         prefix = ''
      end
      i = i + 1
   end

   local n = table.getn(args)

   printf('\n#define FUN_%s(name)\\\n' ..
      'LUA_API int L ## name (lua_State *L) {\\\n', sig)

   if sig == 'v' then
      printf('   (void) L; /* unused */ \\\n')
   end

   for i = 2, n do
      local k, v = i-1, ptypes[args[i]]
      printf('   %s a%d = (%s) %s(L, %d);\\\n', v.type, k, v.type, v.getf, k)
   end

   if ptypes[args[1]].type ~= 'void' then
      printf('   %s(L, (%s) ', ptypes[args[1]].putf, ptypes[args[1]].type);
   else
      printf('   ')
   end

   printf('name(')
   if n > 1 then printf('a1') end
   for i = 3, n do printf(', a%d', i-1) end
   printf(')')

   if ptypes[args[1]].type ~= 'void' then
      printf(');\\\n   return 1;\\\n}\n')
   else
      printf(';\\\n   return 0;\\\n}\n')
   end
end

local signatures = {
   -- signatures from luagl.c
   'BE', 'BuI', 'E', 'IE', 'sE', 'uIZ', 'v', 'vB', 'vB4', 'vD',
   'vD2', 'vD3', 'vD4', 'vD6', 'vE', 'vE2', 'vE2D', 'vE2F', 'vE2p',
   'vE3', 'vEF', 'vEI', 'vEI2Z2IE2p', 'vEI2ZIE2p', 'vEIE2p', 'vEIEp',
   'vEIp', 'vEIuI', 'vEp', 'vEuI', 'vF', 'vF2', 'vF4', 'vI', 'vI2Z2',
   'vI2Z2E', 'vI2Z2E2p', 'vIuH', 'vp', 'vT', 'vuI', 'vuIE', 'vuIZ',
   'vZ2E2p', 'vZ2F4p', 'vZEp', 'vZp',
   -- signatures from luaglut.c
   'Fi2', 'i', 'iD3p6', 'iE', 'IEIZ2E2p', 'IEIZE2p', 'ip', 'ipi',
   'is', 'p', 'ps', 'v', 'vD', 'vD2I2', 'vD4', 'vD4p', 'vD9', 'vDI2',
   'vE', 'vi', 'vI', 'vi2', 'viF3', 'visi', 'vp', 'vpi', 'vps', 'vs',
   'vsi', 'vui',
}

print [[
/* vim: set ts=3 et: */
/* This file is automatically generated by menus.lua */

inline const char * check_string(lua_State *L, int n)
{
   if (lua_type(L,n) != LUA_TSTRING)
      luaL_typerror(L, n, lua_typename(L, LUA_TSTRING));
   return (const char *) lua_tostring(L, n);
}

inline const void * check_lightuserdata(lua_State *L, int n)
{
   if (lua_type(L,n) != LUA_TLIGHTUSERDATA)
      luaL_typerror(L, n, "lightuserdata");
   return (const void *) lua_touserdata(L, n);
}

#define CONSTANT(name)\
   lua_pushstring(L, #name);\
   lua_pushnumber(L, name);\
   lua_settable(L, LUA_GLOBALSINDEX);

#define POINTER(name)\
   lua_pushstring(L, #name);\
   lua_pushlightuserdata(L, name);\
   lua_settable(L, LUA_GLOBALSINDEX);

#define FUN_SPEC(name)

#define FUN(name)\
   lua_register(L, #name, L ## name);
]]

-- Generate numeric OpenGL datatype getters (check_GL*)
for k, v in ipairs(opengl_numeric_types) do
   local tname, fname = ptypes[v].type, ptypes[v].getf
   printf([[
inline %s %s(lua_State *L, int n)
{
   if (lua_type(L,n) != LUA_TNUMBER)
      luaL_typerror(L, n, "number(%s)");
   return (%s) lua_tonumber(L, n);
}

]], tname, fname, tname, tname)
end

-- Generate numeric C datatype getters (check_*)
for k, v in ipairs(c_numeric_types) do
   local tname, fname = ptypes[v].type, ptypes[v].getf
   printf([[
inline %s %s(lua_State *L, int n)
{
   if (lua_type(L,n) != LUA_TNUMBER)
      luaL_typerror(L, n, "number(%s)");
   return (%s) lua_tonumber(L, n);
}

]], tname, fname, tname, tname)
end

-- Generate macros for defined function signatures
table.foreach(signatures, function(k, v) gen_macro(v) end)
