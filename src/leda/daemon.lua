-----------------------------------------------------------------------------
-- Leda Lua API
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

local string,table,leda,kernel,io = string,table,leda,leda.kernel,io
local getmetatable,setmetatable,type,tostring,assert,print,pairs,ipairs,tonumber=
      getmetatable,setmetatable,type,tostring,assert,print,pairs,ipairs,tonumber
local is_graph=leda.leda_graph.is_graph

local socket=require("socket")
local dbg = leda.debug.get_debug("Daemon: ")

local localhost=localhost or "127.0.0.1"
localhost=socket.dns.toip(localhost)
local localport=localport or 9999
localport=tonumber(localport)
local daemons={}
local daemon_socket=nil
local magicbyte='\027'
local default_controller=require("leda.controller.default")

module("leda.daemon")

function get_daemon(host,port)
   port=tonumber(port)
   assert(type(host)=="string",string.format("Invalid daemon hostname (string expected, got %s)",type(host)))
   local ip=socket.dns.toip(host)
   for _,d in ipairs(daemons) do
      if d.host==ip and d.port==port then
         return d
      end
   end
   local d={host=ip,port=port}
   table.insert(daemons,d)
   return d
end

local function is_local(d)
   if d.host==localhost and d.port==localport then
      return true
   end
   return false
end

function start(p_port,controller,has_graph)
   localport=p_port or localport
   daemon_socket=assert(socket.bind("*", localport))
   local ip, port = daemon_socket:getsockname()
   if has_graph~=true then
      io.stderr:write(string.format("Waiting for graph on port '%s'",tostring(port)))
      local client=daemon_socket:accept()
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

      for cl in pairs(gr:clusters()) do
         for _,d in ipairs(cl.daemons) do
            if is_local(d) and #cl.daemons>1 then
               io.stderr:write(string.format("WARNING: Using local daemon for cluster '% s'\n",tostring(cl)))
            end
         end
      end      
      
      local ro_gr=kernel.build_graph(gr,localhost,localport)
      client:send("ACCEPTED\n")
      client:close()
      init(gr,ro_gr,localhost,localport,controller)
   end
end


local function prepare_graph(g,daemon)
   local t={}
   for s in pairs(g:stages()) do
      t[s]={handler=s.handler,init=s.init,pending=s.pending,bind=s.bind}
      s.bind=nil  --no need to transfer bind function for stage
      s.pending={}
      local cl=g:get_cluster(s)
      if not cl:contains(daemon.host,daemon.port) then
         s.handler=""
         s.init=""
      else
--         print("Stage is on daemon",s,daemon.host,daemon.port)
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
   dbg("Sending graph to daemon '%s:%d'",d.host,d.port)
   local client=assert(socket.connect(d.host,d.port))
   client:settimeout(10)
   local t=prepare_graph(g,d)
   local serialized_graph=leda.kernel.encode(g)
   restore_graph(g,t)
   t=nil
   client:send(string.format(magicbyte.."%s\n%d\n",leda._VERSION,#serialized_graph))
   client:send(serialized_graph)
   local response=assert(client:receive())
   assert(response=="ACCEPTED",string.format("Error sending graph to daemon '%s:%d': %s",d.host,d.port,response))
   client:close()
end

function run(g,controller)
   assert(is_graph(g),string.format("Invalid parameter #1 (graph expected, got %s)",type(g)))
   assert(type(localhost)=="string",string.format("Invalid local hostname (string expected, got %s)",type(localhost)))
    
   local d_list={}
   
   for cl in pairs(g:clusters()) do
      if #cl.daemons==0 then
         cl:set_daemon(localhost,localport)
      end
      for _,d in ipairs(cl.daemons) do
         if is_local(d) and #cl.daemons>1 then
            io.stderr:write(string.format("WARNING: Using local daemon for cluster '%s'\n",tostring(cl)))
         end
         d_list[d]=true
      end
   end

   local ro_graph = kernel.build_graph(g,localhost,localport)
   
   start(localport,nil,true)
   
   for d in pairs(d_list) do
      if not is_local(d) then
         send_graph(g,d)
      end
   end
   
   init(g,ro_graph,localhost,localport,controller)
end

function init(g,ro_g,host,port,controller)
   dbg("Daemon started: %s:%d",host,port)
--   ro_g:dump()
   leda.kernel.run(g,ro_g,controller or default_controller,daemon_socket:getfd())
end
