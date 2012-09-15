-----------------------------------------------------------------------------
-- Leda Stage Lua API
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

-----------------------------------------------------------------------------
-- Declare module and import dependencies
-----------------------------------------------------------------------------
local base = _G
local type,pairs,assert,tostring,setmetatable,getmetatable,error,io,print,loadstring=
      type,pairs,assert,tostring,setmetatable,getmetatable,error,io,print,loadstring
local string,table,kernel= string,table,leda.kernel
local leda_connector = require("leda.leda_connector")
local is_connector=leda.leda_connector.is_connector
local new_connector=leda.leda_connector.new_connector
local dbg = leda.debug.get_debug("Stage: ")
local dump = string.dump
local leda=leda

module("leda.leda_stage")

----------------------------------------------------------------------------
-- Stage metatable
-----------------------------------------------------------------------------
local stage = {__index = {}}

-----------------------------------------------------------------------------
-- Stage __tostring metamethod
-----------------------------------------------------------------------------
function stage.__tostring (s) 
   if s.name then 
      return s.name
   else
      return string.format("Stage (%s)",kernel.to_pointer(s)) 
   end
end

-----------------------------------------------------------------------------
-- Stage __index metamethod
-----------------------------------------------------------------------------
local index=stage.__index

-----------------------------------------------------------------------------
-- Add pending data to the stage
-----------------------------------------------------------------------------    
function index.send(self,...)
   local v={...}
   table.insert(self.pending,v)
end

-----------------------------------------------------------------------------
-- method stage:connect([key,]tail,method)
-- Connect two stages at output port 'key', if key is absent, assume it is the 
-- default output (1)
-- the 'tail' argument must be a stage.
-- the method is a function called when the producer stage calls the send
-- method on the new connector output port
-----------------------------------------------------------------------------
function index.connect(head,key,tail,method)
   return function (g) return g:connect(head,key,tail,method) end
end
leda.connect=index.connect

function metatable()
   return stage
end

-----------------------------------------------------------------------------
-- Creates a new stage and returns it
-- param:   't': table used to hold the stage representation
-----------------------------------------------------------------------------
function new_stage(t,init,name,bind,serial)
   local s={}
   if type(t)=="function" then  -- arg1=handler, arg2=init, arg3=name, ...
      s.handler=t
      s.init=init
      assert(type(s.init)=="function" or type(s.init)=="nil",string.format("Stage's init field must be a function or nil",type(s.init)))
      s.name=name
      s.bind=bind
      s.serial=serial
   elseif type(t) == "table" and not is_stage(t) then
      s.handler=t.handler
      assert(type(s.handler)=="function",string.format("Stage's event handler field must be a function (got %s)",type(s.handler)))
      s.init=t.init
      assert(type(s.init)=="function" or type(s.init)=="nil",string.format("Stage's init field must be a function or nil",type(s.init)))
      s.name=t.name
      s.bind=t.bind
      s.serial= t.serial
   elseif is_stage(t) then
      s.handler=kernel.decode(t.handler)
      if t.init then s.init=kernel.decode(t.init) end
      s.bind=t.bind
--      s.name=t.name.."'"
      s.serial=t.serial
   end

   s=setmetatable(s,stage)
 
   s.handler=kernel.encode(s.handler)
   if type(s.init)=="function" then s.init=kernel.encode(s.init) end
   
   s.name=s.name or tostring(s)
   s.pending={}
   return s
end

-----------------------------------------------------------------------------
-- Verify if parameter 's' is a stage
-- (i.e. has the stage metatable)
--
-- returns:       'true' if 's' is a stage
--                'false' if not
-----------------------------------------------------------------------------
function is_stage(s)
   if getmetatable(s)==stage then return true end
   return false
end
