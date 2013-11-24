-----------------------------------------------------------------------------
-- Leda's Connector Lua API
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

-----------------------------------------------------------------------------
-- Declare module and import dependencies
-----------------------------------------------------------------------------
local string,table,kernel = string,table,require'leda.kernel'
local getmetatable,setmetatable,type,tostring,assert,io,error=
      getmetatable,setmetatable,type,tostring,assert,io,error
local debug=require("leda.debug")

local dbg = debug.get_debug("Connector: ")
local dump = string.dump

local leda=require'leda'

--module("leda.leda_connector")

local t={}

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

function t.metatable()
   return connector
end

-----------------------------------------------------------------------------
-- Verify if parameter 'c' is a connector 
-- (i.e. has the connector metatable)
--
-- returns:       'true' if 'c' is a connector
--                'false' if not
-----------------------------------------------------------------------------
function t.is_connector(c)
   if getmetatable(c)==connector then return true end
   return false
end

t.coupled="local"
t.cohort="cohort"
t.decoupled="decoupled"


function index.get_type(c)
   if c.type==t.coupled or c.type==t.decoupled or c.type==t.cohort then
      return c.type
   end
   error(string.format("Unkown connector type '%s'",tostring(c.type)))
end

-----------------------------------------------------------------------------
-- Creates a new connector and returns it
-- param:   'c': table used to hold the connector representation
-----------------------------------------------------------------------------    
function t.new_connector(prod,port,cons,ctype)
   assert(leda.leda_stage.is_stage(prod) or prod==nil,string.format("Parameter #1 must be a stage (got %s)",type(prod)))
   assert(type(port)=="string" or type(port)=="number",string.format("Parameter #2 must be a string or number (got %s)",type(port)))
   assert(leda.leda_stage.is_stage(cons),string.format("Parameter #3 must be a stage (got %s)",type(cons)))
   ctype=ctype or t.decoupled
   
   local c=setmetatable({}, connector)

   c.type=ctype 
   assert(c:get_type(),string.format("Unknown connector type: %s",tostring(ctype)))
  
   c.producer=prod
   c.port=port
   c.consumer=cons

   
   dbg("Created connector '%s'",tostring(c))
   return c
end

return t
