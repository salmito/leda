-- this hack will work for you if you don't have compat-5.1 installed
-- see http://www.keplerproject.org/compat/ on how to set
-- the environment so that the .so/.dll/.bundle is loaded without requiring
-- a file like this
-- This file is not necessary if you use Lua 5.1 - you can remove it.

local loadlib = loadlib or package.loadlib

local function try_loading(name, extension, prefix)
   local path = name
   prefix = prefix or ''
   if extension then path = './' .. name .. '.' .. extension end
   local f = loadlib(path, prefix .. 'luaopen_' .. name)
   if f then f() end
   return f ~= nil
end

local ok = try_loading('luagl', 'so') or
           try_loading('luagl', 'bundle') or
           try_loading('luagl', 'bundle', '_') or
           try_loading('luagl', 'dll') or
           try_loading('luagl')

if not ok then
   print('Cannot load luagl ?!?')
end
