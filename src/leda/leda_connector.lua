-----------------------------------------------------------------------------
-- Leda's Connector Lua API
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

-----------------------------------------------------------------------------
-- Declare module and import dependencies
-----------------------------------------------------------------------------
local string,table,kernel = string,table,leda.kernel
local getmetatable,setmetatable,type,tostring,assert,io=
      getmetatable,setmetatable,type,tostring,assert,io

local dbg = leda.debug.get_debug("Connector: ")
local dump = string.dump
local leda=leda

module("leda.leda_connector")

-----------------------------------------------------------------------------
-- Connector metatable
-----------------------------------------------------------------------------
local index={}
local connector = {__index=index}

-----------------------------------------------------------------------------
-- Connector __tostring metamethod
-----------------------------------------------------------------------------
function connector.__tostring(c)
   if c.port then 
      return tostring(c.port)
   else
      return string.format("Ivalid connector (%s)",kernel.to_pointer(c)) 
   end
end

function metatable()
   return connector
end

-----------------------------------------------------------------------------
-- Verify if parameter 'c' is a connector 
-- (i.e. has the connector metatable)
--
-- returns:       'true' if 'c' is a connector
--                'false' if not
-----------------------------------------------------------------------------
function is_connector(c)
   if getmetatable(c)==connector then return true end
   return false
end

emmit="emmit"
call="call"
fork="fork"

function index.get_type(c)
   if c.method==emmit then
      return "emmit"
   elseif c.method==call then
      return "call"
   elseif c.method==fork then
      return "fork"
   end
   error("Unkown connector type")
end

-----------------------------------------------------------------------------
-- Creates a new connector and returns it
-- param:   'c': table used to hold the connector representation
-----------------------------------------------------------------------------    
function new_connector(prod,port,cons,method)
   assert(leda.leda_stage.is_stage(prod) or prod==nil,string.format("Parameter #1 must be a stage (got %s)",type(prod)))
   assert(type(port)=="string" or type(port)=="number",string.format("Parameter #2 must be a string or number (got %s)",type(port)))
   assert(leda.leda_stage.is_stage(cons),string.format("Parameter #3 must be a stage (got %s)",type(cons)))
   method=method or emmit
--   assert(type(method)=="function",string.format("Parameter #4 (method) must be a function (got %s)",type(method)))
   
   local c=setmetatable({}, connector)
  
   c.producer=prod
   c.port=port
   c.consumer=cons
   c.method=method 
   
   dbg("Created connector '%s'",tostring(c))
   return c
end
