-----------------------------------------------------------------------------
-- Leda Lua API
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

local string,table,leda,kernel,io = string,table,leda,leda.kernel,io
local getmetatable,setmetatable,type,tostring,assert,print,pairs,ipairs,tonumber,error=
      getmetatable,setmetatable,type,tostring,assert,print,pairs,ipairs,tonumber,error
local is_graph=leda.leda_graph.is_graph

local socket=require("socket")
local dbg = leda.debug.get_debug("Process: ")
local default_p=9999
local localhost=localhost or "127.0.0.1"
localhost=socket.dns.toip(localhost)
local l_localport=localport or default_p
l_localport=tonumber(l_localport)
local processes={}
local process_socket=nil
local magicbyte='\027'
local default_controller=require("leda.controller.default")

require('leda.controller.fixed_thread_pool')

module("leda.process")

default_port=l_localport

function get_process(host,port)
   if not host and not port then
      host=localhost
      port=l_localport
   end
   port=tonumber(port)
   assert(type(host)=="string",string.format("Invalid hostname (string expected, got %s)",type(host)))
   local ip=socket.dns.toip(host)
   for _,d in ipairs(processes) do
      if d.host==ip and d.port==port then
         return d
      end
   end
   local d={host=ip,port=port}
   table.insert(processes,d)
   return d
end

local function is_local(d)
   if d.host==localhost and (d.port==l_localport or d.port==nil) then
      return true
   end
   return false
end

function start(p_port,maxpar,controller,has_graph)
   if type(p_port)=="table" then
      local t=p_port
      p_port=t.port
      maxpar=t.maxpar
      controller=t.controller
   end
   l_localport=p_port or l_localport
   process_socket=assert(socket.bind("*", l_localport))
   local ip, port = process_socket:getsockname()
   if has_graph~=true then
      io.stderr:write(string.format("Waiting for graph on port '%s'\n",tostring(port)))
      local client=process_socket:accept()
      local peer_ip,peer_port=client:getpeername()
      client:settimeout(10)
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
      init(gr,ro_gr,localhost,l_localport,controller,maxpar)
   end
end


local function prepare_graph(g,process)
   local t={}
   for s in pairs(g:stages()) do
      t[s]={handler=s.handler,init=s.init,pending=s.pending,bind=s.bind}
      s.bind=nil  --no need to transfer bind function for stage
      s.pending={}
      local cl=g:get_cluster(s)
      if not cl:is_local(process.host,process.port) then
         s.handler=""
         s.init=""
      end
   end
   return t
end

local function restore_graph(g,t)
   for s,r in pairs(t) do
      s.bind=r.bind
      s.pending=r.pending
      s.handler=r.handler
      s.init=r.init
   end
end

local function send_graph(g,d)
   dbg("Sending graph to process '%s:%d'",d.host,d.port)
   local client,err=socket.connect(d.host,d.port)
   if not client then 
      
      error(string.format("Error connecting to '%s:%d': %s",d.host,d.port,err))
   end
   client:settimeout(10)
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

function run(g,localport,maxpar,controller)
   assert(is_graph(g),string.format("Invalid parameter #1 (graph expected, got %s)",type(g)))
   assert(type(localhost)=="string",string.format("Invalid local hostname (string expected, got %s)",type(localhost)))
   if type(localport)=="table" then
      local t=localport
      localport=t.localport
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
   start(l_localport,nil,nil,true)   
   init(g,ro_graph,localhost,l_localport,controller,maxpar)
end

function init(g,ro_g,host,port,controller,maxpar)
--   io.stderr:write(string.format("Starting graph\n",host,port))
--   ro_g:dump()
   maxpar=maxpar or -1
   leda.kernel.run(g,ro_g,controller or default_controller,maxpar,process_socket:getfd())
end
