-----------------------------------------------------------------------------
-- Leda Graph Lua API
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

-----------------------------------------------------------------------------
-- Declare module and import dependencies
-----------------------------------------------------------------------------
local base = _G
local tostring,type,assert,pairs,setmetatable,getmetatable,print,error,ipairs,tonumber =
      tostring,type,assert,pairs,setmetatable,getmetatable,print,error,ipairs,tonumber
local string,table,kernel,io=string,table,leda.kernel,io
local dbg = leda.debug.get_debug("Graph: ")
local is_connector=leda.leda_connector.is_connector
local is_stage=leda.leda_stage.is_stage
local stage_meta=leda.leda_stage.metatable

local leda=leda

module("leda.leda_cluster")

----------------------------------------------------------------------------
-- Cluster metatable
-----------------------------------------------------------------------------
local cluster_metatable = { 
   __index={}
}

function metatable() return cluster_metatable end

-----------------------------------------------------------------------------
-- Cluster __tostring metamethod
-----------------------------------------------------------------------------
function cluster_metatable.__tostring(c) 
   local s = "{"
   local sep = ""
   for e in pairs(c) do
      if is_stage(e) then
         s = s .. sep .. tostring(e)
         sep = ", "
      end
   end
   return s .. "}"
end


-----------------------------------------------------------------------------
-- Graph __index metamethod
-----------------------------------------------------------------------------
local index=cluster_metatable.__index

function new_cluster(...)
   local t={...}
   if type(t[1])=='table' and not is_stage(t[1]) then
      t=t[1]
   end
   
   local cluster=setmetatable({},cluster_metatable)
   cluster.process_addr={}
      
   for i,s in pairs(t) do
      if is_stage(s) then
         cluster[s]=true
      end
   end
   
   return cluster
end

function is_cluster(c) 
  if getmetatable(c)==cluster_metatable then return true end
  return false
end

function index.contains(cluster,stage)
   return cluster[stage]==true
end

function index.size(cluster)
   assert(is_cluster(cluster),string.format("Invalid parameter #1 (cluster expected, got %s)",type(cluster)))
   local count=0
   for s in pairs(cluster) do
      if is_stage(s) then
         count=count+1
      end
   end
   return count
end

function index.union(c1,c2) 
   if (type(c1)=='table' or is_stage(c1)) and not is_cluster(c1) then
      c1=new_cluster(c1)
   end
   if (type(c2)=='table' or is_stage(c2)) and not is_cluster(c2) then
      c2=new_cluster(c2)
   end

   assert(is_cluster(c1),string.format("Invalid parameter #1 (cluster expected, got %s)",type(c1)))
   assert(is_cluster(c2),string.format("Invalid parameter #2 (cluster expected, got %s)",type(c2)))
   local res=new_cluster()
   for s in pairs(c1) do 
      if is_stage(s) then
         res[s]=true 
      end
   end
   for s in pairs(c2) do 
     if is_stage(s) then
         res[s]=true
      end
   end
   return res
end

function index.sub(c1,c2)
    if (type(c1)=='table' or is_stage(c1)) and not is_cluster(c1) then
      c1=new_cluster(c1)
   end
   if (type(c2)=='table' or is_stage(c2)) and not is_cluster(c2) then
      c2=new_cluster(c2)
   end

   assert(is_cluster(c1),string.format("Invalid parameter #1 (cluster expected, got %s)",type(c1)))
   assert(is_cluster(c2),string.format("Invalid parameter #2 (cluster expected, got %s)",type(c2)))
   local res=new_cluster()
   for s in pairs(c1) do
      if is_stage(s) then
         if not c2[s] then
            res[s]=true
         end
      end
   end

   return res
end

function index.intersection (c1,c2)
   if (type(c1)=='table' or is_stage(c1)) and not is_cluster(c1) then
      c1=new_cluster(c1)
   end
   if (type(c2)=='table' or is_stage(c2)) and not is_cluster(c2) then
      c2=new_cluster(c2)
   end



   local res=new_cluster()
   for s in pairs(c1) do
     if is_stage(s) then
         res[s] = c2[s]
     end
   end
   return res
end

cluster_metatable.__add = index.union
stage_meta().__add = index.union
cluster_metatable.__sub = index.sub
stage_meta().__sub= index.sub
cluster_metatable.__mul = index.intersection
stage_meta().__mul= index.mul

function index.has_serial(cluster)
   assert(is_cluster(cluster),string.format("Invalid parameter #1 (Cluster expected, got %s)",type(cluster)))
   for s in pairs(cluster) do
      if s.serial then return true end
   end
   return false
end

function index.is_local(cluster,host,port)
   for _,d in ipairs(cluster.process_addr) do
      if d.host==host and d.port==port then
         return true
      end
   end
   return false
end

function index.set_process(cluster,host,port)
   assert(is_cluster(cluster),string.format("Invalid parameter #1 (Cluster expected, got %s)",type(cluster)))
--   assert(type(host)=="string",string.format("Invalid parameter #2 (String expected, got %s)",type(host)))
   port=tonumber(port) or leda.process.default_port

   if cluster:has_serial() and #cluster.process_addr>0 then
      error("Cannot add more than one process for a cluster with a serial stage")
   end
   cluster.process_addr={leda.process.get_process(host,port)}
end

function index.add_process(cluster,host,port)
   assert(is_cluster(cluster),string.format("Invalid parameter #1 (Cluster expected, got %s)",type(cluster)))
--   assert(type(host)=="string",string.format("Invalid parameter #2 (String expected, got %s)",type(host)))
   port=tonumber(port) or leda.process.default_port

   assert(cluster.process_addr,"A process must be set before calling this funcion")

   if cluster:has_serial() and #cluster.process_addr>0 then
      error("Cannot add more than one process for a cluster with a serial stage")
   end
   table.insert(cluster.process_addr,leda.process.get_process(host,port))
end

