-----------------------------------------------------------------------------
-- Leda Graph Lua API
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

-----------------------------------------------------------------------------
-- Declare module and import dependencies
-----------------------------------------------------------------------------
local base = _G
local tostring,type,assert,pairs,setmetatable,getmetatable,print,error,ipairs =
      tostring,type,assert,pairs,setmetatable,getmetatable,print,error,ipairs
local string,table,kernel,io=string,table,leda.kernel,io
local dbg = leda.debug.get_debug("Graph: ")
local is_connector=leda.leda_connector.is_connector
local new_connector=leda.leda_connector.new_connector
local is_stage=leda.leda_stage.is_stage
local default_controller=require("leda.controller.default")
local leda=leda

module("leda.leda_graph")

----------------------------------------------------------------------------
-- Graph metatable
-----------------------------------------------------------------------------
local graph_metatable = { 
   __index={}
}

----------------------------------------------------------------------------
-- Cluster metatable
-----------------------------------------------------------------------------
local cluster_metatable = { 
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
-- Cluster __tostring metamethod
-----------------------------------------------------------------------------
function cluster_metatable.__tostring(c) 
   if c.name then 
      return c.name
   else
      return string.format("Cluster (%s)",kernel.to_pointer(c)) 
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
function index.add_connector(self,c)
   if type(c)=='function' then
      c=c(self)
   end
   if self:contains(c) then return end
   assert(is_graph(self),string.format("Invalid parameter #1 ('graph' expected, got '%s')",type(self)))
   assert(is_connector(c),string.format("Invalid parameter #2 ('connector' expected, got '%s')",type(c)))
   dbg("Adding connector '%s' to graph '%s'",tostring(c),tostring(self))
   self.conns[c]=true
end

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
   gr.cl={}
   gr.name=gr.name or tostring(gr)
   gr.default_cluster=gr:create_cluster(gr.name.."_main")
   for k, v in pairs(gr) do
      --if value is a connector, add it to graph
      if is_connector(v) then
         gr:add_connector(v)
      elseif type(v)=='function' then
         local c=v(gr)
         assert(is_connector(c),string.format("Connector constructor returned an invalid value (%s)",type(c)))
         gr:add_connector(c)
      else --ignore other values
         --io.stderr:write(string.format("WARNING: Ignoring parameter #%d of graph '%s'\n",i,gr.name))
      end
   end
   if gr.start then
      assert(is_stage(gr.start),string.format("Graph 'start' field must be a stage (got %s)",type(gr.start)))
      local c=new_connector(nil,'start',gr.start,leda.emmit)
      gr:add_connector(c)
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

function index.contains(g,s)
   if is_stage(s) then
      local ss=g:stages()
      return ss[s]==true
   elseif is_connector(s) then
      return g.conns[s]==true
   end
   error(string.format("Invalid parameter type: %s",type(s)))
end

function index.stages(g)
   local stages={}
   for c,_ in pairs(g.conns) do
      if c.producer then stages[c.producer]=true end
      stages[c.consumer]=true
   end
   return stages
end

function index.clusters(g)
   local clusters={}
   clusters[g.default_cluster]=g.default_cluster
   for _,c in pairs(g.cl) do
      clusters[c]=c
   end
   return clusters
end

function index.get_cluster(g,s)
   assert(is_graph(g),string.format("Invalid parameter #1 type (Graph expected, got %s)",type(g)))
   assert(is_stage(s),string.format("Invalid parameter #1 type (Stage expected, got %s)",type(s)))
   if g.cl[s] then
      return g.cl[s]
   end
   return g.default_cluster
end

function index.get_ports(g,s)
   assert(is_graph(g),string.format("Invalid parameter #1 type (Graph expected, got %s)",type(g)))
   assert(is_stage(s),string.format("Invalid parameter #1 type (Stage expected, got %s)",type(s)))
   if type(g.outputs[s])=='table' then
      return g.outputs[s]
   end
   return {}
end

function index.connectors(g)
   local ret={}
   for c in pairs(g.conns) do if c.producer then ret[c]=true end end
   return ret
end

function index.daemons(g)
   local ret={}
   for cl in pairs(g:clusters()) do 
      for _,d in ipairs(cl.daemons) do
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

function index.count_daemons(g)
   local count=0
   for k,v in pairs(g:daemons()) do count=count+1 end
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
   error("Module 'leda.utils.plot' not loaded.'")
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
   for c in pairs(g:connectors()) do
      if c.producer then
         if g:get_cluster(c.producer)~=g:get_cluster(c.consumer) and c:get_type()~="emmit" then
            error(string.format("Stages '%s' and '%s' cannot be on different clusters because they exchange threads with each other",tostring(c.producer),tostring(c.consumer)))
         end
      end
   end
   for s in pairs(g:stages()) do
      if type(s.bind)=="function" then
         s.bind(g:get_ports(s) or {})
      end
   end
   return true
end

-----------------------------------------------------------------------------
-- Proxy to run a graph (passing it to the kernel.run function
-- if no controller was provided, use the default one
-----------------------------------------------------------------------------
function index.run(g,controller)
   assert(is_graph(g),string.format("Invalid parameter #1 (graph expected, got '%s')",type(g)))
   local flag,err=g:verify()
   if flag then
      return leda.daemon.run(g,controller,host,port)
   end
   error(err)
end

-----------------------------------------------------------------------------
-- Dump a graph
-----------------------------------------------------------------------------
function index.dump(g)
   print('==== DUMP Stages ====')
   for s,_ in pairs(g:stages()) do 
      print(string.format("Stage: name='%s' pending='%d' serial='%s' cluster='%s'",tostring(s),#s.pending,tostring(s.serial==true),tostring(g.cl[s])))
      for k,v in pairs(g:get_ports(s)) do print(string.format("\tOutput: %s -> %s\t",tostring(k),tostring(v.consumer))) end
   end
   print('==== DUMP Connectors ====')
   for c,_ in pairs(g:connectors()) do 
      print(string.format("Connector: name='%s.%s' consumer='%s' type='%s'",tostring(c.producer),tostring(c),tostring(c.consumer),c:get_type()))
   end
   print('==== DUMP Clusters ====')

   for c,_ in pairs(g:clusters()) do
      print(string.format("Cluster: name='%s' serial='%s'",tostring(c),tostring(c:is_serial()==true)))
      if c.daemons then
         for i,d in ipairs(c.daemons) do
            print(string.format("\tDaemon #%d: '%s:%d'",i,d.host,d.port))
         end
      end
   end

   dbg('========')
end


function index.create_cluster(g,...)
   assert(is_graph(g),string.format("Invalid parameter #1 type (Graph expected, got %s)",type(g)))
   local stages={...}
   local i=2
   local cluster=setmetatable({graph=g,daemons={}},cluster_metatable)
   for _,s in ipairs(stages) do
       if type(s)=='string' and not cluster.name then cluster.name=s 
       else
       assert(is_stage(s),string.format("Invalid parameter #%d type (stage expected, got %s)",i,type(s)))
       i=i+1
       assert(g:contains(s),string.format("Stage '%s' is not on the graph '%s'",tostring(s),tostring(g)))
       
      if g.cl[s] then
         error(string.format("Stage '%s' is already in another cluster",tostring(s)))
      end
      cluster[s]=true
      g.cl[s]=cluster
      end
   end
   return cluster
end

function is_cluster(c) 
  if getmetatable(c)==cluster_metatable then return true end
  return false
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
      setmetatable(cl,cluster_metatable)
   end
   return g
end

function cluster_metatable.__index.is_serial(cluster)
   assert(is_cluster(cluster),string.format("Invalid parameter #1 (Cluster expected, got %s)",type(cluster)))
   for s,_ in pairs(cluster) do
      if is_stage(s) and s.serial then return true end
   end
   return false
end

function cluster_metatable.__index.contains(cluster,host,port)
   for _,d in ipairs(cluster.daemons) do
      if d.host==host and d.port==port then
         return true
      end
   end
   return false
end

function cluster_metatable.__index.add_daemon(cluster,host,port)
   assert(is_cluster(cluster),string.format("Invalid parameter #1 (Cluster expected, got %s)",type(cluster)))
   assert(type(host)=="string",string.format("Invalid parameter #2 (String expected, got %s)",type(host)))
   port=port or 9999
   cluster.daemons=cluster.daemons or {}
   if cluster:is_serial() and #cluster.daemons>0 then
      error("Cannot add more than one daemon for a cluster with a serial stage")
   end
   table.insert(cluster.daemons,leda.daemon.get_daemon(host,port))
end

function cluster_metatable.__index.set_daemon(cluster,host,port)
   assert(is_cluster(cluster),string.format("Invalid parameter #1 (Cluster expected, got %s)",type(cluster)))
   cluster.daemons={}
   cluster:add_daemon(host,port)
end
