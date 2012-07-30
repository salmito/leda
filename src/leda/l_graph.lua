-----------------------------------------------------------------------------
-- Leda Graph Lua API
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

-----------------------------------------------------------------------------
-- Declare module and import dependencies
-----------------------------------------------------------------------------
local base = _G
local tostring,type,assert,pairs,setmetatable,getmetatable,print =
      tostring,type,assert,pairs,setmetatable,getmetatable,print
local string,table,kernel=string,table,leda.kernel
local dbg = leda.debug.get_debug("Graph: ")
local is_connector=leda.l_connector.is_connector
local is_stage = leda.l_stage.is_stage
local default_controller=require("leda.controller.default")
local dump = string.dump

module("leda.l_graph")

----------------------------------------------------------------------------
-- Graph metatable
-----------------------------------------------------------------------------
local graph_metatable = { 
   __index={}
}

-----------------------------------------------------------------------------
-- Graph __tostring metamethod
-----------------------------------------------------------------------------
function graph_metatable.__tostring(g) 
   if g.name then 
      return g.name
   else
      return string.format("Graph (%s)",kernel.to_pointer(g)) 
   end
end

-----------------------------------------------------------------------------
-- Graph __index metamethod
-----------------------------------------------------------------------------
local index=graph_metatable.__index

-----------------------------------------------------------------------------
-- Verify if the value 'm' is already a member of 't'
-- returns:       'true' if 'm' is a member of 't'
--                'false' if not
-----------------------------------------------------------------------------
local function is_member(t,m)
   for _,v in pairs(t) do if v==m then return true end end
   return false
end

----------------------------------------------------------------------------
-- Add stage to a graph, 's' must be a stage.
--
-- If a stage is already on the graph, nothing is done
-----------------------------------------------------------------------------
function index.add_stage(self, s)
   assert(is_stage(s),"Invalid parameter ('stage' expected)")
   if not is_member(self.stages,s) then
      dbg("Adding stage '%s' to graph '%s'",tostring(s),tostring(self))
      table.insert(self.stages,s)
   end
end

----------------------------------------------------------------------------
-- Add connector to a graph, 'c' must be a connector.
--
-- If a connector is already on the graph, nothing is done
-----------------------------------------------------------------------------
local function add_connector(self,c)
   assert(is_connector(c),string.format("Invalid parameter ('connector' expected, got '%s')",type(c)))
   if not is_member(self.connectors,c) then
      dbg("Adding connector '%s' to graph '%s'",tostring(c),tostring(self))
         --Dump the send function
      if type(c.sendf)=="function" then
         c.sendf=dump(c.sendf)
      end
      table.insert(self.connectors,c)
   end
end

function index.add(self, s)
   assert(is_stage(s),"Invalid parameter ('stage' expected)")
   if not is_member(self.stages,s) then
      dbg("Adding stage '%s' to graph '%s'",tostring(s),tostring(self))
      table.insert(self.stages,s)
   end
end


-----------------------------------------------------------------------------
-- Creates a new graph and returns it
-- param:   't': table used to hold the graph representation
-----------------------------------------------------------------------------  
function graph(t)
  local name 
  --If the first field of the table is a string, use it as its name
  if type(t[1])=="string" then
     name=t[1]
     table.remove(t,1)
  end
  

   t.stages={}
   t.connectors={}
   local g = setmetatable(t,graph_metatable)
   
   --if name was not defined, define as tostring
   g.name=name or tostring(g)
   
   --iterate through the fields of t
   for k, v in pairs(t) do
      --bypass graph specific fields
      if  k=='name' or k=='stages' or k=='connectors' then
      --if value is a stage, add it to graph
      elseif is_stage(v) then
         g:add_stage(v)
      --every other value is ignored
      else
         dbg("Invalid field for graph '%s': field '%s' type '%s'",tostring(g),tostring(k),type(v))
      end
   end
  
  return g
end

-----------------------------------------------------------------------------
-- Verify if parameter 'g' is a graph
-- (i.e. has the graph metatable)
--
-- returns:       'true' if 'g' is a graph
--                'false' if not
-----------------------------------------------------------------------------
function is_graph(g) 
  if getmetatable(g)==graph_metatable then return true end
  return false
end

-----------------------------------------------------------------------------
-- Verify if a graph 'g' is a well-formed leda graph
--
-- Initialize all connectors of the graph
-- Calls every stage's bind function afterwards
--
-- returns:       'true' if 'g' is a well formed graph
--                fails with an error message if not
-----------------------------------------------------------------------------
function index.verify(g)
   --reset previous producers and consumers
   for _,c in pairs(g.connectors) do
      c.producers={}
      c.consumers={}   
   end
   g.connectors={}   
   --initialize connectors
   for _,s in pairs(g.stages) do
      add_connector(g,s.input)
      s.input:add_consumer(s)
      for _,c in pairs(s.output) do
         add_connector(g,c)
         c:add_producer(s)
      end
      if #s.input.producers ==0 and #s.input.pending==0 then
         dbg("WARNING: Connector '%s' does not have any producers and there is no data peding",tostring(s.input))
      end
   end

   -- Calling bind function after connectors has been initialized
   for _,s in pairs(g.stages) do
      if type(s.bind)=="function" then
         s:bind()
      end
   end

   return true
end

-----------------------------------------------------------------------------
-- Dump a graph
-----------------------------------------------------------------------------
function index.dump(g)
   print('==== DUMP Stages ====')
   for id,s in pairs(g.stages) do 
      print(string.format("Stage: id='%d' name='%s' input='%s'",id,tostring(s),tostring(s.input)))
      for k,v in pairs(s.output) do print(string.format("Output: %s -> %s\t",tostring(k),tostring(v))) end
   end
   print('==== DUMP Connectors ====')
   for id,c in pairs(g.connectors) do 
      print(string.format("Connector: id='%d' name='%s' pending='%d'",id,tostring(c),#c.pending))
      print("producers:")
      for k,v in pairs(c.producers) do print(string.format("\t %s -> %s",tostring(k),tostring(v))) end
      print("consumers:")
      for k,v in pairs(c.consumers) do print(string.format("\t %s -> %s",tostring(k),tostring(v))) end
   end
   dbg('========')
   
end

-----------------------------------------------------------------------------
-- Proxy to run a graph (passing it to the kernel.run function
-- if no controller was provided, use the default one
-----------------------------------------------------------------------------
function index.run(self,c1,...)
   local v,err=index.verify(self)
--   self:dump()
   dbg("Running graph '%s'...",tostring(self))
   if v then 
      return kernel.run(self,c1 or default_controller,...) 
   end
   return nil, err
end

--[[function index.add_field=function (self,field)
      if type(v)=="function" then
         v(g)
      elseif l_stage.is_stage(v) then
         g:add_stage(v)
      elseif l_connector.is_connector(v) then
         g:add_connector(v)
      else
         dbg("Invalid field for graph '%s': field '%s' type '%s'",tostring(g),tostring(k),type(v))
      end   
      end,

--]]
