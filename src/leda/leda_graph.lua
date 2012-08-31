-----------------------------------------------------------------------------
-- Leda Graph Lua API
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

-----------------------------------------------------------------------------
-- Declare module and import dependencies
-----------------------------------------------------------------------------
local base = _G
local tostring,type,assert,pairs,setmetatable,getmetatable,print,error,ipairs,unpack =
      tostring,type,assert,pairs,setmetatable,getmetatable,print,error,ipairs,unpack
local string,table,kernel,io=string,table,leda.kernel,io
local dbg = leda.debug.get_debug("Graph: ")
local is_connector=leda.leda_connector.is_connector
local new_connector=leda.leda_connector.new_connector
local is_stage=leda.leda_stage.is_stage
local is_cluster=leda.leda_cluster.is_cluster
local leda=leda

module("leda.leda_graph")

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

----------------------------------------------------------------------------
-- Add connector to a graph
-- If a connector is already on the graph, nothing is done
-----------------------------------------------------------------------------
function index.add(self,c)
   if type(c)=='function' then
      c=c(self)
   end
   if self:contains(c) then return end
   assert(is_graph(self),string.format("Invalid parameter #1 ('graph' expected, got '%s')",type(self)))
   assert(is_connector(c),string.format("Invalid parameter #2 ('connector' expected, got '%s')",type(c)))
   dbg("Adding connector '%s' to graph '%s'",tostring(c),tostring(self))
   self.conns[c]=true
end
index.add_connector=add

-----------------------------------------------------------------------------
-- Create a new graph and returns it
-- param:   't': table used to hold the graph representation
-----------------------------------------------------------------------------  
function graph(...)
   local t={...}
   if type(t[1]=='table') and not is_graph(t[1]) then
      t=t[1]
   end
   
   if type(t[1])=="string" then
      t.name=t.name or t[1]
      table.remove(t,1)
   end

   local gr = setmetatable(t,graph_metatable)
   gr.conns={}
   gr.outputs={}
   gr.name=gr.name or tostring(gr)

   for i, v in pairs(gr) do
      --if value is a connector, add it to graph
      if is_connector(v) then
         gr:add(v)
      elseif type(v)=='function' then
         local c=v(gr)
         assert(is_connector(c),string.format("Connector constructor returned an invalid value (%s)",type(c)))
         gr:add(c)
      else --ignore other values
         dbg("WARNING: Ignoring parameter of graph '%s' (type %s)\n",gr.name,type(v))
      end
   end
   
   if gr.start then
      assert(is_stage(gr.start),string.format("Graph 'start' field must be a stage (got %s)",type(gr.start)))
      local c=new_connector(nil,'start',gr.start)
      gr:add(c)
      assert(gr:contains(gr.start),"Graph start field is not a stage in the graph")
   end
   return gr
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
index.is_graph=is_graph

function index.set_start(g,s)
   assert(is_stage(s),string.format("Invalid parameter (stage expected, got %s)",type(s)))
   if not g:contains(s) then error(string.format("Stage '%s' not defined on graph '%s'",s,g)) end
   for c in pairs(g:connectors()) do
      if c.producer == nil then
         c.consumer=stage
         return true
     end
   end
   local c=new_connector(nil,'start',s)
   g:add(c)
   return true
end

function index.contains(g,s)
   assert(is_graph(g),string.format("Invalid parameter #1 type (Graph expected, got %s)",type(g)))
   if is_stage(s) then
      local stages=g:stages()
      return stages[s]==true
   elseif is_connector(s) then
      return g.conns[s]==true
   end
   error(string.format("Invalid parameter type (stage or connector expected, got %s)",type(s)))
end

function index.stages(g)
   assert(is_graph(g),string.format("Invalid parameter #1 type (Graph expected, got %s)",type(g)))
   local stages={}
   for c,_ in pairs(g.conns) do
      if c.producer then stages[c.producer]=true end
      stages[c.consumer]=true
   end
   return stages
end

function index.all(g)
   local res=leda.cluster()
   for s in pairs(g:stages()) do
      res[s]=true
   end
   return res
end

function index.clusters(g)
   local clusters={}
   cl=g.cluster or {}
   for c in pairs(cl) do
      clusters[c]=true
   end
   return clusters
end

function index.part(g,...)
   assert(is_graph(g),string.format("Invalid parameter #1 type (graph expected, got %s)",type(g)))

   for s in pairs(g:stages()) do
      if type(s.bind)=="function" then
         s.bind(g:get_output_ports(s))
      end
   end

   local t={...}
   
   local all=leda.cluster()
   g.cluster=nil
   local c={}
   local res={}
   
   for _,cl in pairs(t) do
      if is_stage(cl) then
         cl=leda.cluster(cl)
      end
      if type(cl)=='table' and not is_cluster(cl) then
         cl=leda.cluster(cl)
      end
      if is_cluster(cl) then
         local i=all*cl
         assert(i:size()==0,"Invalid cluster, stages "..tostring(i).." are already clustered")
         for s in pairs(cl) do
            if is_stage(s) then
               for key,c in pairs(g:get_output_ports(s)) do
                  if c.type~='decoupled' then
                     assert(cl:contains(c.consumer),"Invalid cluster, stages '"..tostring(s).."' and '"..tostring(c.consumer).."' cannot be on different clusters")
                  end
               end
            end
         end
         all=all+cl
         c[cl]=true
         table.insert(res,cl)
      end
   end
   assert((g:all()-all):size()==0,"Invalid configuration, stages "..tostring(g:all()-all).." must be clustered")
   assert((all-g:all()):size()==0,"Invalid cluster, stages "..tostring(all-g:all()).." are not on the graph")
   g.cluster=c
   return unpack(res)
end

function index.get_cluster(g,s)
   assert(is_graph(g),string.format("Invalid parameter #1 type (Graph expected, got %s)",type(g)))
   assert(is_stage(s),string.format("Invalid parameter #1 type (Stage expected, got %s)",type(s)))
   for c in pairs(g:clusters()) do
      if c:contains(s) then return c end
   end
   return nil,"Cluster not found"
end

function index.get_output_ports(g,s)
   assert(is_graph(g),string.format("Invalid parameter #1 type (Graph expected, got %s)",type(g)))
   assert(is_stage(s),string.format("Invalid parameter #1 type (Stage expected, got %s)",type(s)))
   if type(g.outputs[s])=='table' then
      return g.outputs[s]
   end
   g.outputs[s]={}
   return g.outputs[s]
end

function index.connectors(g)
   local ret={}
   for c in pairs(g.conns) do if c.producer then ret[c]=true end end
   return ret
end

function index.processes(g)
   local ret={}
   for cl in pairs(g:clusters()) do 
      for _,d in ipairs(cl.process_addr) do
         ret[d]=true
      end
   end
   return ret
end

function index.count_connectors(g)
   local count=0
   for k,v in pairs(g:connectors()) do count=count+1 end
   return count
end

function index.count_processes(g)
   local count=0
   for k,v in pairs(g:processes()) do count=count+1 end
   return count
end

function index.count_stages(g)
   local count=0
   for k,v in pairs(g:stages()) do count=count+1 end
   return count
end

function index.count_clusters(g)
   local count=0
   for k,v in pairs(g:clusters()) do count=count+1 end
   return count
end

function index.send(g,...)
   if g.start and is_stage(g.start) then 
      return g.start:send(...)
   end
   error(string.format("Start stage not defined for graph '%s'",tostring(g)))
end

function index.plot(g,out)
   if leda.plot_graph then 
      return leda.plot_graph(g,out)
   end
   error("Module 'leda.utils.plot' must be loaded.'")
end

-----------------------------------------------------------------------------
-- method stage:connect([key,]tail,method)
-- Connect two stages at output port 'key', if key is absent, assume it is the 
-- default output (1)
-- the 'tail' argument must be a stage.
-- the method is a function called when the producer stage calls the send
-- method on the new connector output port
-----------------------------------------------------------------------------
function index.connect(g,head,key,tail,method)
   assert(is_graph,"Parameter #1 must be a graph")
   assert(is_stage(head),"Parameter #2 must be a stage")
   if is_stage(key) then
      method=tail
      tail=key
      key=1
   end
   assert(is_stage(tail),string.format("connector tail must be a stage (got '%s')",type(tail)))
   assert(type(key)=="number" or type(key)=="string",string.format("Output port key must be a number or string (got %s)",type(key)))
--   if method then 
--      assert(type(method)=="function",string.format("Parameter #5 (method) must be a function (got %s)",type(method)))
--   end
   
   local c=nil
   g.outputs[head] = g.outputs[head] or {}
   if g.outputs[head][key] then
      assert(is_connector(g.outputs[head][key]),"Invalid output")
      io.stderr:write(string.format("Stage: Warning: Overwriting output port '%s' of stage '%s' of graph '%s'\n",tostring(key),tostring(head),tostring(g)))
      g.outputs[head][key].producer=head
      g.outputs[head][key].consumer=tail
      c=g.outputs[head][key]
   end
   c=c or new_connector(head,key,tail,method)
   g.outputs[head][key]=c
   return c
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
   local all=leda.cluster()
   for cl in pairs(g:clusters()) do
         i=cl*all
         assert(i:size()==0,"Invalid cluster, stages "..tostring(i).." are already clustered")
         all=all+cl
   end
   assert((all-g:all()):size()==0,"Invalid cluster, stages "..tostring(all-g:all()).." are not on the graph")
   assert((g:all()-all):size()==0,"Invalid configuration, stages "..tostring(g:all()-all).." are not clustered")
   
   for c in pairs(g:connectors()) do   
      if c.producer then
         if g:get_cluster(c.producer)~=g:get_cluster(c.consumer) and c:get_type()~="decoupled" then
            error(string.format("Stages '%s' and '%s' are coupled and cannot be on different clusters",tostring(c.producer),tostring(c.consumer)))
         end
      end
   end
   return true
end

-----------------------------------------------------------------------------
-- Proxy to run a graph (passing it to the kernel.run function
-- if no controller was provided, use the default one
-----------------------------------------------------------------------------
function index.run(g,localport,controller)
   assert(is_graph(g),string.format("Invalid parameter #1 (graph expected, got '%s')",type(g)))
   if not g.cluster then
      g:part(g:all()):set_process()
   end
   local flag,err=g:verify()
   if flag then
      return leda.process.run(g,controller,localport)
   end
   error(err)
end

-----------------------------------------------------------------------------
-- Dump a graph
-----------------------------------------------------------------------------
function index.dump(g)
   print('==== DUMP Stages ====')
   for s,_ in pairs(g:stages()) do 
      print(string.format("Stage: name='%s' pending='%d' serial='%s' cluster='%s'",tostring(s),#s.pending,tostring(s.serial==true),tostring(g:get_cluster(s))))
      for k,v in pairs(g:get_output_ports(s)) do print(string.format("\tOutput: %s -> %s\t",tostring(k),tostring(v.consumer))) end
   end
   print('==== DUMP Connectors ====')
   for c,_ in pairs(g:connectors()) do 
      print(string.format("Connector: name='%s.%s' consumer='%s' type='%s'",tostring(c.producer),tostring(c),tostring(c.consumer),c:get_type()))
   end
   print('==== DUMP Clusters ====')

   for c,_ in pairs(g:clusters()) do
      print(string.format("Cluster: name='%s' serial='%s'",tostring(c),tostring(c:has_serial()==true)))
      if c.process_addr then
         for i,d in ipairs(c.process_addr) do
            print(string.format("\tDaemon #%d: '%s:%d'",i,d.host,d.port))
         end
      end
   end

   dbg('========')
end

function restore_metatables(g)
   setmetatable(g,graph_metatable)
   for c in pairs(g.conns) do
      setmetatable(c,leda.leda_connector.metatable())
   end
   for s in pairs(g:stages()) do
      setmetatable(s,leda.leda_stage.metatable())
   end
   for cl in pairs(g:clusters()) do
      setmetatable(cl,leda.leda_cluster.metatable())
   end
   return g
end
