-----------------------------------------------------------------------------
-- Leda's Connector Lua API
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

-----------------------------------------------------------------------------
-- Declare module and import dependencies
-----------------------------------------------------------------------------
local string,table,kernel = string,table,leda.kernel
local getmetatable,setmetatable,type,tostring,assert=
      getmetatable,setmetatable,type,tostring,assert

local dbg = leda.debug.get_debug("Connector: ")
local dump = string.dump
local leda=leda

module("leda.l_connector")

-----------------------------------------------------------------------------
-- Connector metatable
-----------------------------------------------------------------------------
local connector = {__index = {}}

-----------------------------------------------------------------------------
-- Connector __tostring metamethod
-----------------------------------------------------------------------------
function connector.__tostring(c)
   if c.name then 
      return c.name
   else
      return string.format("Connector (%s)",kernel.to_pointer(c)) 
   end
end

-----------------------------------------------------------------------------
-- Connector __index metamethod
-----------------------------------------------------------------------------
local index=connector.__index

-----------------------------------------------------------------------------
-- Add a producer to a connector, 'producer' must be a stage
-----------------------------------------------------------------------------
function index.add_producer(self,producer)
   assert(leda.l_stage.is_stage(producer),"'producer' must be a stage")
   dbg("Adding producer '%s' on connector '%s'",tostring(producer),tostring(self))
   table.insert(self.producers,producer)
end

-----------------------------------------------------------------------------
-- Add a consumer to a connector, 'consumer' must be a stage
-----------------------------------------------------------------------------
function index.add_consumer(self,consumer)
   assert(leda.l_stage.is_stage(consumer),"'consumer' must be a stage")
   dbg("Adding consumer '%s' on connector '%s'",tostring(consumer),tostring(self))
   table.insert(self.consumers,consumer)
end

-----------------------------------------------------------------------------
-- Add pending data to the connector
-----------------------------------------------------------------------------    
function index.send(self,...)
   local v={...}
   table.insert(self.pending,v)
end

-----------------------------------------------------------------------------
-- Function to throws an event for each consumer of the connector
-----------------------------------------------------------------------------    
emmit_func=
function(consumers,...) 
   for _,c in pairs(consumers) do
      __emmit(c,...);
   end
end

-----------------------------------------------------------------------------
-- Function to pass the thread for each consumer of the connector in order
-- and wait for them to complete
-----------------------------------------------------------------------------    
call_func=
function(consumers,...) 
   for _,c in pairs(consumers) do
      __call(c,...);
   end
end

-----------------------------------------------------------------------------
-- Function that and throws an event with the continuation of the handler
-- and pass the thread for each consumer of the connector in order
-----------------------------------------------------------------------------    
emmit_self_call_func=
function(consumers,...) 
   for _,c in pairs(consumers) do
      __emmit_self_call(c,...);
   end
end

-----------------------------------------------------------------------------
-- Creates a new connector and returns it
-- param:   'c': table used to hold the connector representation
-----------------------------------------------------------------------------    
function new_connector(c)
   c=c or {}
   if type(c[1])=="string" then
     c.name=c.name or c[1]
     table.remove(c,1)
   end
   c=setmetatable(c or {}, connector)
   
   c.consumers=c.consumers or {}
   c.producers=c.producers or {}
   c.pending=c.pending or {}
   c.sendf=c.sendf or emmit_func
   
   --Dump the send function
   if type(c.sendf)=="function" then
      c.sendf=dump(c.sendf)
   end
   
   dbg("Created connector '%s'",tostring(c))
   return c
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
