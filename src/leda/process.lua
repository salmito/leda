-----------------------------------------------------------------------------
-- Leda Lua API
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

local string,table,leda,kernel,io = string,table,leda,leda.kernel,io

local getmetatable,setmetatable,type,tostring,assert,print,pairs,ipairs,tonumber,error,unpack=
      getmetatable,setmetatable,type,tostring,assert,print,pairs,ipairs,tonumber,error,unpack
      
local leda_graph=require 'leda.leda_graph'
local is_graph=leda_graph.is_graph
local get_default_localhost=leda_graph.get_localhost
local socket=require("socket")
local debug=require("leda.debug")
local dbg = debug.get_debug("Process: ")
local default_p=9999
local l_localport=localport or default_p
l_localport=tonumber(l_localport)
local processes={}
local process_socket=nil
local magicbyte='\024'
local default_controller=require("leda.controller.default")

--preload some controllers
require('leda.controller.thread_pool')
require('leda.controller.singlethread')
--require('leda.controller.profiler')

--module("leda.process")
local t={}

t.default_port=l_localport


local localhost=localhost or '127.0.0.1'--get_default_localhost()
localhost=socket.dns.toip(localhost) or localhost

function t.get_process(host,port)
   if not host and not port then
      host=localhost
      port=l_localport
   end
   port=tonumber(port)
   assert(type(host)=="string",string.format("Invalid hostname (string expected, got %s)",type(host)))
   local ip=socket.dns.toip(host) or host
   for _,d in ipairs(processes) do
      if d.host==ip and d.port==port then
         return d
      end
   end
   local d={host=ip,port=port}
   table.insert(processes,d)
   return d
end

function is_local(d)
   if d.host==localhost and (d.port==l_localport or d.port==nil) then
      return true
   end
   return false
end

function t.get_localhost()
   return localhost
end

function t.get_localport()
   return l_localport
end

local function init(g,ro_g,host,port,controller,maxpar)
--   io.stderr:write(string.format("Starting graph\n",host,port))
--   ro_g:dump()
   maxpar=maxpar or -1
   
   for stage in pairs(g:stages()) do
      if stage.autostart then
         if type(stage.autostart)=='table' then
            stage:send(unpack(stage.autostart))
         else
            stage:send(stage.autostart)
         end
      end
   end
   
   return leda.kernel.run(g,ro_g,controller and leda.controller.default  or default_controller,maxpar,process_socket:getfd())
end
t.init=init

local function start(p_port,p_host,controller,maxpar,has_graph)
   if type(p_port)=="table" then
      local t=p_port
      p_port=t.port
      p_host=t.host
      maxpar=t.maxpar
      controller=t.controller
   end
   l_localport=p_port or l_localport
   localhost=p_host and socket.dns.toip(p_host) or localhost
   process_socket=assert(socket.bind("*", l_localport))
   local ip, port = process_socket:getsockname()
   if has_graph~=true then
      io.stderr:write(string.format("Process '%s:%d' Waiting for graph\n",localhost,tostring(port)))
      local client=process_socket:accept()
      local peer_ip,peer_port=client:getpeername()
--      client:settimeout(10)
      -- receive the line
      local magicnumber=assert(client:receive(1))
      if magicnumber~=magicbyte then
         error("Invalid request")
      end
      local version=assert(client:receive())
      if version~=leda._VERSION then
         error("Invalid Leda version")
      end
      local size=assert(client:receive("*l"))
      local g_str=assert(client:receive(size))
       
      local gr=leda.kernel.decode(g_str)
      leda.leda_graph.restore_metatables(gr)
      dbg("Received graph '%s' from '%s'",tostring(gr),peer_ip)
      local ro_gr=kernel.build_graph(gr,localhost,l_localport)
      client:send("ACCEPTED\n")
      client:close()
      return init(gr,ro_gr,localhost,l_localport,controller,maxpar)
   end
end
t.start=start

local function prepare_graph(g,process)
   local t={}
   for s in pairs(g:stages()) do
      t[s]={}
      for k,v in pairs(s) do
         t[s][k]=v
         s[k]=nil
      end
      s.name=t[s].name
      s.pending={}
      s.stagesid=t[s].stagesid
      s.connectorsid=t[s].connectorsid
      s.clustersid=t[s].clustersid
      local cl=g:get_cluster(s)
      if cl:is_local(process.host,process.port) then
         s.handler=t[s].handler
         s.init=t[s].init
      else
         s.handler=""
         s.init=""
      end
   end
   return t
end

local function restore_graph(g,t)
   for s,r in pairs(t) do
      for k,v in pairs(r) do
         s[k]=v
      end
   end
end

local function send_graph(g,d)
   dbg("Sending graph to process '%s:%d'",d.host,d.port)
   local client,err=socket.connect(d.host,d.port)
   if not client then 
      error(string.format("Error connecting to '%s:%d': %s",d.host,d.port,err))
   end
  --client:settimeout(10)
   local t=prepare_graph(g,d)
   local serialized_graph=leda.kernel.encode(g)
   restore_graph(g,t)
   t=nil
   client:send(string.format(magicbyte.."%s\n%d\n",leda._VERSION,#serialized_graph))
   client:send(serialized_graph)
   local response=assert(client:receive())
   assert(response=="ACCEPTED",string.format("Error sending graph to '%s:%d': %s",d.host,d.port,response))
   client:close()
end

function t.run(g,localport,maxpar,controller)
   assert(is_graph(g),string.format("Invalid parameter #1 (graph expected, got %s)",type(g)))
   assert(type(localhost)=="string",string.format("Invalid local hostname (string expected, got %s)",type(localhost)))
   if type(localport)=="table" then
      local t=localport
      localport=t.port
      maxpar=t.maxpar
      controller=t.controller
   end
   l_localport=localport or l_localport
   local d_list={}
   
   for cl in pairs(g:clusters()) do
      if #cl.process_addr==0 then
         error(string.format("Cluster '%s' does not have any process",tostring(cl)))
      end
      for _,d in ipairs(cl.process_addr) do
         if d_list[d]==true then
            error(string.format("Process '%s:%d' is already running a cluster",d.host,d.port))
         end
         d_list[d]=true
      end
   end

   local ro_graph = kernel.build_graph(g,localhost,l_localport)
 
   for d in pairs(d_list) do
      if not is_local(d) then
         send_graph(g,d)
      end
   end
   start(l_localport,nil,nil,nil,true)   
   return init(g,ro_graph,localhost,l_localport,controller,maxpar)
end



return t
