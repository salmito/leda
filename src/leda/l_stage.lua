-----------------------------------------------------------------------------
-- Leda Stage Lua API
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

-----------------------------------------------------------------------------
-- Declare module and import dependencies
-----------------------------------------------------------------------------
local base = _G
local type,pairs,assert,tostring,setmetatable,getmetatable,error=
      type,pairs,assert,tostring,setmetatable,getmetatable,error
local string,table,kernel= string,table,leda.kernel
local is_connector=leda.l_connector.is_connector
local connector=leda.l_connector.new_connector
local dbg = leda.debug.get_debug("Stage: ")
local dump = string.dump
local leda=leda

module("leda.l_stage")

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
-- Set the input of a stage. 'input' must be a connector
-----------------------------------------------------------------------------
function index.set_input(self,input)
   assert(is_connector(input),"parameter must be a connector")
   self.input=input
end

-----------------------------------------------------------------------------
-- Proxy to send pending data to the input connector of a stage
-----------------------------------------------------------------------------
function index.send(self,...)
   self.input:send(...)
end

-----------------------------------------------------------------------------
-- Proxy to set the method sendf of the input connector of a stage
-----------------------------------------------------------------------------
function index.input_method(self,sendf) 
    self.input.sendf=sendf
end

function index.method(self,key,sendf) 
   key=key or 1
   if is_connector(self.output[key]) then
      self.output[key].sendf=sendf
   else 
      error("Invalid connector type")
   end
end

-----------------------------------------------------------------------------
-- method stage:connect([key,]tail)
-- Connect two stages at output key, if key is absent, assume it is the 
-- fist integer key of the output vector
-- the 'tail' argument can be a connector to be used or a stage, in which case 
-- its input connector is used.
-----------------------------------------------------------------------------
function index.connect(head,key,tail)
   if not tail then tail = key key = 1 end
   if not is_stage(head) then error("Invalid argument") end
   if is_stage(tail) then
      head.output[key] = head.output[key] or 
                         connector{tostring(head).."."..tostring(key)} 
      tail.input=head.output[key]
   elseif is_connector(tail) then 
      head.output[key]=tail
   else error("Invalid argument") end
end
leda.connect=index.connect

-----------------------------------------------------------------------------
-- Creates a new stage and returns it
-- param:   't': table used to hold the stage representation
-----------------------------------------------------------------------------
function new_stage(t)
   --assertions
   assert(type(t)=="table","Invalid parameter ('table' expected, got '%s'",type(t))
   
   if is_stage(t[1]) then
      t.handler=t.handler or t[1].handler
      t.init=t.init or t[1].init
      t.input=t.input or t[1].input
      t.output=t.output or t[1].output
      t.name=t.name or t[1].name
      t.serial=t.serial or t[1].serial
      t.backpressure=t.backpressure or t[1].backpressure
   elseif type(t[1])=="table" then
      t.handler=t.handler or t[1].handler
      t.init=t.init or t[1].init
      t.input=t.input or t[1].input
      t.output=t.output or t[1].output
      t.name=t.name or t[1].name
      t.serial=t.serial or t[1].serial
      t.backpressure=t.backpressure or t[1].backpressure
   elseif type(t[1])=="string" then
      t.name=t[1]
   elseif type(t[1])=="function" then
      t.handler=t[1]
   end
  
   assert(type(t.handler)=="function","Stage's 'handler' field must be a function")

   --Dump the event handler defined for the stage
   if type(t.handler)=="function" then
      t.handler=kernel.encode(t.handler)
   end
   
   assert(type(t.handler)=="string","Stage 'handler' field must be a lua chunk")
   
  	--container for the stage
  	local s=setmetatable(t,stage)
   
  
--   if s.init then
--      assert(type(s.init)=="function","Stage handler must be a function")
--   end

   --if a init field was defined, dump it 
   if s.init and type(s.init)=="function" then
      s.init=dump(s.init)
   end
   
   assert(type(s.init)=="string" or not s.init,"Stage 'init' field must be a lua chunk")
 
   --Handling connectors
   if s.input then
      assert(is_connector(s.input),"Invalid input for stage: %s",tostring(s.input))
   else
      dbg("Creating new connector for the input of stage '%s'",tostring(s))
      s.input=connector{string.format("%s_input",tostring(s))}
   end
   
   --Create an empty container for output, if not already created
   s.output=s.output or {}
  	s.name=s.name or tostring(s)

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

--[[function stage.__add(s1,s2)
   base.print("adding",s1,s2)
   if is_stage(s2) then
      s1:set_output({s2.input})
   else
      error("Invalid operand")
   end
   return s1
end

function stage.__concat(s1,s2)
   base.print("concating",s1,s2)
   if is_stage(s2) then
      s1.input=s2.input
   else
      error("Invalid operand")
   end
   return s1
end]]--
