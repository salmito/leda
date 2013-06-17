-----------------------------------------------------------------------------
-- Leda Graph Lua API
-----------------------------------------------------------------------------
--[[ @name leda
module "leda.graph"
]]-----------------------------------------------------------------------------

-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura

-----------------------------------------------------------------------------
-- Declare module and import dependencies
-----------------------------------------------------------------------------
local base = _G
local tostring,type,assert,pairs,setmetatable,getmetatable,print,error,ipairs,unpack =
      tostring,type,assert,pairs,setmetatable,getmetatable,print,error,ipairs,unpack
local string,table,kernel,io=string,table,leda.kernel,io
local debug=require("leda.debug")
local dbg = debug.get_debug("Graph: ")
local leda_connector = require("leda.leda_connector")
local is_connector=leda_connector.is_connector
local new_connector=leda_connector.new_connector
local leda_stage = require("leda.leda_stage")
local is_stage=leda_stage.is_stage
local leda_cluster = require("leda.leda_cluster")
local is_cluster=leda_cluster.is_cluster
local leda=leda
local kernel=require 'leda.kernel'
--module("leda.leda_graph")

local t={}

----------------------------------------------------------------------------
-- Graph metatable
-----------------------------------------------------------------------------
local graph_metatable = { 
   __index={}
}

-----------------------------------------------------------------------------
-- Graph __tostring metamethod
-- @name metatable.__tostring
-- @param g A leda.graph
-----------------------------------------------------------------------------
function graph_metatable.__tostring(g) 
   if g.name then 
      return g.name
   else
      return string.format("Graph (%s)",kernel.to_pointer(g)) 
   end
end

function t.get_localhost(interface)
	local h=kernel.hostname()
	if interface then
		return h.ipv4.interface
	end
	for ifname,ip in pairs(h.ipv4) do
		if ifname~='lo' then
			return ip
		end
	end
	--IPv4 interface not found, searching for IPv6
	for ifname,ip in pairs(h.ipv6) do
		if ifname~='lo' then
			return ip
		end
	end
	--Nothing found, returning localhost
	return "127.0.0.1"
end

local localhost = localhost or t.get_localhost()
-----------------------------------------------------------------------------
-- Verify if parameter 'g' is a graph
-- (i.e. has the graph metatable)
-- @name graph.is_graph
-- @return       'true' if 'g' is a graph
--                'false' if not
-----------------------------------------------------------------------------
function t.is_graph(g) 
  if getmetatable(g)==graph_metatable then return true end
  return false
end

local is_graph=t.is_graph

-----------------------------------------------------------------------------
-- Graph __index metamethod
-----------------------------------------------------------------------------
local index=graph_metatable.__index

----------------------------------------------------------------------------
-- Add connector to a graph
-- If a connector is already on the graph, nothing is done
-- If a different connector if added on an already connected output port,
-- a warn is issued
-- @name graph:add
-- @param c Connector to be added to the graph
-----------------------------------------------------------------------------
function index.add(self,c)
   if type(c)=='function' then
      c=c(self)
   end
   assert(is_graph(self),string.format("Invalid parameter #1 ('graph' expected, got '%s')",type(self)))
   assert(is_connector(c),string.format("Invalid parameter #2 ('connector' expected, got '%s')",type(c)))
   if self:contains(c) then return c end
   dbg("Adding connector '%s' to graph '%s'",tostring(c),tostring(self))
   self.conns[c]=true
   return c
end
index.add_connector=add

local add_table=nil

local function add_item(gr,i,v)
      if i=='start' then
         assert(is_stage(gr.start),string.format("Graph 'start' field must be a stage (got %s)",type(gr.start)))
         local c=new_connector(nil,'start',gr.start)
         gr:add(c)
         assert(gr:contains(gr.start),"Graph start field is not a stage in the graph")
      elseif is_connector(v) then
         gr:add(v)
      elseif type(v)=='function' then
         local c=v(gr)
         assert(is_connector(c),string.format("Connector constructor returned an invalid value (%s)",type(c)))
         gr[i]=c
         gr:add(c)
      elseif type(v)=='table' then
      	add_table(gr,v)
      else --ignore other values
         dbg("WARNING: Ignoring parameter of graph '%s' (type %s)\n",gr.name,type(v))
      end
end
  
add_table=function (gr,t)
	assert(not is_stage(t),"Error, trying to add a stage to a graph without a connector")
	for i,v in ipairs(t) do
		add_item(gr,i,v)
	end
end
 
local function new_graph(...)
   local p={...}

   if type(p[1])=='table' then
      p=p[1]
   end
   
   if type(p[1])=="string" then
--      p.name=p.name or p[1]
      table.remove(p,1)
   end

   local gr = setmetatable(p,graph_metatable)
   gr.conns={}
   gr.outputs={}
   gr.name=gr.name or tostring(gr)


   for i, v in pairs(gr) do
      --if value is a connector, add it to graph
		add_item(gr,i,v)
   end
   
   return gr
end

function t.graph(...)
   local g={...}
   if type(g[1])=='string' and #g==1 then -- stage "name" {...}
      local name=g[1]
      return function(...) local g=new_graph(...) g.name=name return g end
   end
   return new_graph(...)
end

index.is_graph=is_graph

----------------------------------------------------------------------------
-- Define a start stage for the graph
-- @name graph:set_start
-- @param s Stage to be used as the start of the pipeline
-----------------------------------------------------------------------------
function index.set_start(g,s)
   assert(is_stage(s),string.format("Invalid parameter (stage expected, got %s)",type(s)))
--   if not g:contains(s) then error(string.format("Stage '%s' not defined on graph '%s'",s,g)) end
   for c in pairs(g:connectors()) do
      if c.producer == nil then
         c.consumer=stage
         return true
     end
   end
   local c=new_connector(nil,'start',s)
   g:add(c)
   g.start=s
   return true
end

----------------------------------------------------------------------------
-- Checks if a stage or a connector is present on a graph
-- @name graph:contains
-- @param s Stage or connector to be checked
-- @return true if the object is on the graph, false if not
-----------------------------------------------------------------------------
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

----------------------------------------------------------------------------
-- Get a set of stages inside a graph
-- @name graph:stages
-- @return A table with a key for each stage of the graph
-- @usage for s in pairs(g:stages()) do ...
-----------------------------------------------------------------------------
function index.stages(g)
   assert(is_graph(g),string.format("Invalid parameter #1 type (Graph expected, got %s)",type(g)))
   local stages={}
   for c,_ in pairs(g.conns) do
      if c.producer then stages[c.producer]=true end
      stages[c.consumer]=true
   end
   return stages
end

----------------------------------------------------------------------------
-- Get a cluster with all stages of the graph
-- @name graph:all
-- @return A cluster with all stages of the graph
-----------------------------------------------------------------------------
function index.all(g)
   local res=leda.cluster()
   for s in pairs(g:stages()) do
      res[s]=true
   end
   return res
end

----------------------------------------------------------------------------
-- Get a set of clusters defined for the graph
-- @name graph:clusters
-- @return A table with a key for each cluster of the graph
-- @usage for cl in pairs(g:clusters()) do ...
-----------------------------------------------------------------------------
function index.clusters(g)
   local clusters={}
   cl=g.cluster or {}
   for _,c in pairs(cl) do
      clusters[c]=true
   end
   return clusters
end

----------------------------------------------------------------------------
-- Partition a graph
-- @name graph:part
-- @param ... A set of clusters
-- @return The partitioned graph
-- @usage g:part(g:all())
-----------------------------------------------------------------------------
function index.part(g,...)
   assert(is_graph(g),string.format("Invalid parameter #1 type (graph expected, got %s)",type(g)))

   for s in pairs(g:stages()) do
      if type(s.bind)=="function" then
         s.bind(g:get_output(s),s,g)
      end
   end

   local t={...}
   
   local all=leda.cluster()
   g.cluster=nil
   local c={}
   
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
               for key,c in pairs(g:get_output(s)) do
                  if c.type~='decoupled' then
                     assert(cl:contains(c.consumer),"Invalid cluster, stages '"..tostring(s).."' and '"..tostring(c.consumer).."' cannot be on different clusters")
                  end
               end
            end
         end
         all=all+cl
         table.insert(c,cl)
      end
   end
   assert((g:all()-all):size()==0,"Invalid configuration, stages "..tostring(g:all()-all).." must be clustered")
   assert((all-g:all()):size()==0,"Invalid cluster, stages "..tostring(all-g:all()).." are not on the graph")
   g.cluster=c
   
   return g
end

----------------------------------------------------------------------------
-- Map each cluster of a partitioned graph into processes
-- @name graph:map
-- @param ... Strings representing each process address
-- @return The mapped graph
-- @usage g:part(g:all()):map('host.domain.com:9999')
-----------------------------------------------------------------------------
function index.map(g,...)
   assert(is_graph(g),string.format("Invalid parameter #1 type (graph expected, got %s)",type(g)))
   assert(g.cluster,"Graph is not partitioned")
   local args={...}
      for i,t in ipairs(args) do
         if type(t)=='string' then t={t} end
         assert(type(t)=="table",string.format("Invalid parameter #%d type (table expected, got %s)",i,type(t)))
         for j,proc in ipairs(t) do
            if j==1 then
               g.cluster[i]:set_process(proc)
            else
               g.cluster[i]:add_process(proc)
            end
         end
      end
      local d_list={}
      for cl in pairs(g:clusters()) do
         if #cl.process_addr==0 then
            g.cluster=nil
            error(string.format("Cluster '%s' does not have any process",tostring(cl)))
         end
         for _,d in ipairs(cl.process_addr) do
            if d_list[d]==true then
               g.cluster=nil
               error(string.format("Process '%s:%d' is already running a cluster",d.host,d.port))
            end
            d_list[d]=true
         end
      end
      return g
   end

----------------------------------------------------------------------------
-- Get the cluster of a stage
-- @name graph:get_cluster
-- @param s A stage to search
-- @return A cluster with the passed stage
-----------------------------------------------------------------------------
function index.get_cluster(g,s)
   assert(is_graph(g),string.format("Invalid parameter #1 type (Graph expected, got %s)",type(g)))
   assert(is_stage(s),string.format("Invalid parameter #1 type (Stage expected, got %s)",type(s)))
   for c in pairs(g:clusters()) do
      if c:contains(s) then return c end
   end
   return nil,"Cluster not found"
end

----------------------------------------------------------------------------
-- Get the output ports of a stage defined on the current graph
-- @name graph:get_output
-- @param s A stage to search for its output
-- @return A cluster with the passed stage
-----------------------------------------------------------------------------
function index.get_output(g,s)
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
   local res=require 'leda.utils.plot'
   return res.plot_graph(g,out)
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
function index.run(g,...)
   assert(is_graph(g),string.format("Invalid parameter #1 (graph expected, got '%s')",type(g)))
   do local sum=0
      for k,v in pairs(g:stages()) do sum=sum+1 break end
      assert(sum>0,string.format("Error: the graph is empty."))
   end
   if not g.cluster then
      g:part(g:all()):map(localhost)
   end
   local flag,err=g:verify()
   if flag then
      return leda.process.run(g,...)
   end
   error(err)
end
-----------------------------------------------------------------------------
-- Get the unique id of a stage
-----------------------------------------------------------------------------
function index.getid(g,s)
   assert(is_graph(g),string.format("Invalid parameter #1 (graph expected, got '%s')",type(g)))
   assert(g.stagesid,"Graph not running")
   assert(is_stage(s),string.format("Invalid parameter #2 (stage expected, got '%s')",type(s)))
   return g.stagesid[s]
end


-----------------------------------------------------------------------------
-- Dump a graph
-----------------------------------------------------------------------------
function index.dump(g)
   print('==== DUMP Stages ====')
   for s,_ in pairs(g:stages()) do 
      print(string.format("Stage: name='%s' pending='%d' serial='%s' cluster='%s'",tostring(s),#s.pending,tostring(s.serial==true),tostring(g:get_cluster(s))))
      for k,v in pairs(g:get_output(s)) do print(string.format("\tOutput: %s -> %s\t",tostring(k),tostring(v.consumer))) end
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
            print(string.format("\tProcess #%d: '%s:%d'",i,d.host,d.port))
         end
      end
   end

   dbg('========')
end

function t.restore_metatables(g)
   setmetatable(g,graph_metatable)
   for c in pairs(g.conns) do
      setmetatable(c,leda_connector.metatable())
   end
   for s in pairs(g:stages()) do
      setmetatable(s,leda_stage.metatable())
   end
   for cl in pairs(g:clusters()) do
      setmetatable(cl,leda_cluster.metatable())
   end
   return g
end

return t
