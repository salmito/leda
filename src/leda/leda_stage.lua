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
local is_connector=leda_connector.is_connector
local new_connector=leda_connector.new_connector
local dbug=require("leda.debug")
local dbg = dbug.get_debug("Stage: ")
local dump = string.dump
local leda=leda

--module("leda.leda_stage")
local t={}

----------------------------------------------------------------------------
-- Stage metatable
-----------------------------------------------------------------------------
local stage = {__index = {}}

-----------------------------------------------------------------------------
-- Stage __tostring metamethod
-----------------------------------------------------------------------------
function stage.__tostring (s) 
   if s.name then 
      return string.format("%s (%s)",s.name,kernel.to_pointer(s))
   else
      return string.format("Stage (%s)",kernel.to_pointer(s)) 
   end
end

-----------------------------------------------------------------------------
-- Stage __index metamethod
-----------------------------------------------------------------------------
local index=stage.__index

-----------------------------------------------------------------------------
-- Verify if parameter 's' is a stage
-- (i.e. has the stage metatable)
--
-- returns:       'true' if 's' is a stage
--                'false' if not
-----------------------------------------------------------------------------
local function is_stage(s)
   if getmetatable(s)==stage then return true end
   return false
end
t.is_stage = is_stage

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
t.connect=index.connect

function t.metatable()
   return stage
end

-----------------------------------------------------------------------------
-- Creates a new stage and returns it
-- param:   't': table used to hold the stage representation
-----------------------------------------------------------------------------
function t.new_stage(t,init,name,bind,serial)
   local s={}
   if type(t)=="function" or type(t)=="string" then  -- arg1=handler, arg2=init, arg3=name, ...
      s.handler=t
      s.init=init
      assert(type(s.init)=="function" or type(s.init)=='string' or type(s.init)=="nil",string.format("Stage's init field must be a function or nil",type(s.init)))
      s.name=name
      s.bind=bind
      s.serial=serial
   elseif type(t) == "table" and not is_stage(t) then
   	if type(t[1])=="string" then
   		t.name=t.name or t[1]
   	end
      s.handler=t.handler
      assert(type(s.handler)=="function" or type(s.handler=="string"),string.format("Stage's event handler field must be a function (got %s)",type(s.handler)))
      s.init=t.init
      assert(type(s.init)=="function" or type(s.init)=='string' or type(s.init)=="nil",string.format("Stage's init field must be a function or nil",type(s.init)))
      s.name=t.name
      s.bind=t.bind
      s.serial=t.serial or t.stateful
      s.autostart=t.autostart
   elseif is_stage(t) then
      s.handler=t.handler
      if t.init then s.init=t.init end
      s.bind=t.bind
      s.serial=t.serial
      s.name=t.name.."'"
      s.autostart=t.autostart
      s.pending={}
      s=setmetatable(s,stage)
      return s
   end

   assert(type(s.handler)=="function" or type(s.handler)=="string","Invalid handler type (string or function expected)")

   s=setmetatable(s,stage)
   local f_handler=s.handler
   if type(f_handler)=="function" then
      local upname,env = debug.getupvalue (s.handler, 1)
      if upname == '_ENV' then
         debug.setupvalue (s.handler, 1,{})
      end
      s.handler=kernel.encode(s.handler)
      debug.setupvalue (f_handler, 1,env)
   else 
       s.handler=kernel.encode(s.handler)
   end
   
   if type(s.init)=="function" or type(s.init)=="string" then s.init=kernel.encode(s.init) end
   
   s.name=s.name or tostring(s)
   s.pending={}
   return s
end


return t
