-----------------------------------------------------------------------------
-- Leda Graph Lua API
-- @author Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

local base = _G
local tostring,type,assert,pairs,setmetatable,getmetatable,print,error,ipairs,tonumber =
      tostring,type,assert,pairs,setmetatable,getmetatable,print,error,ipairs,tonumber
local string,table,kernel,io=string,table,leda.kernel,io
local dbg = leda.debug.get_debug("Graph: ")
local leda_connector = require("leda.leda_connector")
local is_connector=leda_connector.is_connector
local leda_stage = require("leda.leda_stage")
local is_stage=leda_stage.is_stage
local stage_meta=leda_stage.metatable

local leda=leda

--module("leda.leda_cluster")
local t={}

local cluster_metatable = { 
   __index={}
}

function metatable() return cluster_metatable end

function cluster_metatable.__tostring(c) 
   local s = "{"
   local sep = ""
   for e in pairs(c) do
      if is_stage(e) then
         s = s .. sep .. tostring(e.name)
         sep = ", "
      end
   end
   return s .. "}"
end


local index=cluster_metatable.__index

local function new_cluster(...)
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
t.new_cluster=new_cluster


function t.is_cluster(c) 
  if getmetatable(c)==cluster_metatable then return true end
  return false
end
local is_cluster=t.is_cluster

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

local sm=stage_meta()
cluster_metatable.__add = index.union
sm.__add = index.union
cluster_metatable.__sub = index.sub
sm.__sub= index.sub
cluster_metatable.__mul = index.intersection
sm.__mul= index.mul
sm=nil

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

   cluster.process_addr={}
   cluster:add_process(host,port)   
end

function index.add_process(cluster,host,port)
   assert(is_cluster(cluster),string.format("Invalid parameter #1 (Cluster expected, got %s)",type(cluster)))
   if type(host)=="string" then
      local h,p=string.gmatch(host,"([%w%.]+):(%d+)")()
      if h then
         host=h
         port=p
      end
   end
   port=port or leda.process.default_port
   port=tonumber(port)

   assert(cluster.process_addr,"A process must be set before calling this funcion")

   if cluster:has_serial() and #cluster.process_addr>0 then
      error("Cannot add more than one process for a cluster with a serial stage")
   end
   cluster.process_addr=cluster.process_addr or {}
   table.insert(cluster.process_addr,leda.process.get_process(host,port))
end

return t
