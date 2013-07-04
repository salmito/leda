-----------------------------------------------------------------------------
-- Leda simple http thread pool controller
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

local base = _G
local debug=require("leda.debug")
local dbg = debug.get_debug("Controller: HTTP: ")
local kernel=leda.kernel
local table,ipairs,pairs,print=table,ipairs,pairs,print
local default_thread_pool_size=kernel.cpu()
local socket=require "socket"
local mime=require"mime"

local default_port=8080

local t={}
local pool_size=default_thread_pool_size
local th={}
local server={}
local init_time=nil

local graph=nil

local auth=nil

local get_index,get_css,get_jquery,get_jquery_flot, get_jquery_arbor

local totmem=kernel.memory()

local get_smaps=function()
	local mem=kernel.smaps()
	local str={'{'}
	local pref=nil
	for k,v in pairs(mem) do
		table.insert(str,(pref or '')..'"')
		table.insert(str,k)
		table.insert(str,'":'..v)
		pref=','
	end
	table.insert(str,',"total":'..(totmem/1024)..',"percentage":'..((mem.Rss*100/(totmem/1024)))..'}')
	return table.concat(str)
end


local last_t={}
local last_t2={}
local function get_stats()
   local cluster=nil
   local clusters="["
   local i=0
   local stats,cstats=kernel.stats()
   for cl in pairs(graph:clusters()) do
      if i > 0 then clusters=clusters..',' end i=i+1
      clusters=clusters..'{"host": "'..cl.process_addr[1].host..
                         '","port": '..cl.process_addr[1].port..', "local": '
      if cl:is_local(leda.process.get_localhost(),leda.process.get_localport()) then
         clusters=clusters..' 1}'
         cluster=cl
      else
         clusters=clusters..' 0}'
      end
   end
   clusters=clusters..']'
   local connectors="["
   i=0
   for key,c in ipairs(cstats) do
      local last=last_t2[key] or {0,0}
      last_t2[key]={c.events_pushed,kernel.gettime()}
      local events=c.events_pushed-last[1]
      local time=kernel.gettime()-last[2]
      if i > 0 then connectors=connectors..',' end i=i+1
      connectors=connectors..string.format('{"name": "%s.%s->%s", "events": %d,"latency": %.6f, "throughput": %.6f}',
         tostring(c.producer), 
         tostring(c.key),
         tostring(c.consumer),
         c.events_pushed,
         c.average_latency,
         events/time
         )
   end
   connectors=connectors..']'
   
   local stages="["
   i=0
   if cluster then for s in pairs(cluster) do if s~="process_addr" then
      if i > 0 then stages=stages..',' end i=i+1
      local k=graph:getid(s)
      local v=stats[tonumber(k)+1]
      local last=last_t[k] or {0,0}
      last_t[k]={v.events_pushed,kernel.gettime()}
      local events=v.events_pushed-last[1]
      local time=kernel.gettime()-last[2]
      stages=stages..string.format(
      '{"id": %d, "name": "%s", "ready": %d, "maxpar": %d, "events": %d, "queue": %d, "executed": %d, "error": %d, "latency": %.6f, "throughput": %.6f}',
      k,
      tostring(v.name),
      v.active,
      v.maxpar,
      v.events_pushed, 
      v.event_queue_size, 
      v.times_executed, 
      v.errors, 
      v.average_latency, 
      events/time
      )
   end end end
   stages=stages..']'
   
   kernel.stats_latency_reset()

   local now=leda.gettime()-init_time
   local thread_pool_size=kernel.thread_pool_size()
   local ready_queue_size,tmp=0,kernel.ready_queue_size()
   local active_threads=0
   if tmp<0 then
      ready_queue_size=0
      active_threads=thread_pool_size+tmp
   else
      active_threads=thread_pool_size 
      ready_queue_size=tmp 
   end
   local ready_queue_capacity=kernel.ready_queue_capacity()
   
   --local stats,cstats=kernel.stats()
   
	return string.format([[{
"thread_pool_size": %d,
"ready_queue_size": %d,
"ready_queue_capacity": %d,
"active_threads": %d,
"uptime": %.6f,
"clusters": %s,
"connectors": %s,
"stages": %s
}]],
thread_pool_size,
ready_queue_size,
ready_queue_capacity,
active_threads,
now,
clusters,
connectors,
stages
)
end

function stdresp(str)
	local res="HTTP/1.1 " ..
		str ..
		"\r\n" ..
		"Date: " .. os.date() .. "\r\n" ..
		"Server: Leda HTTP controller\r\n"
	if auth then
	   res=res..'WWW-Authenticate: Basic realm="Leda"'
	end
	return res
end

local function send(clt,resp)
				 		local sent,a,err=0,nil,nil
				 		local lsent=0
				 		while sent<#resp do
						   a,err,lsent=clt:send(string.sub(resp,sent))
						   --print(lsent,err,sent,#resp,a)
						   if lsent==nil and err~='timeout' then break end
			            if type(lsent)=='number' then		   sent=sent+lsent end
						end

end

local function http_server(port)
	dbg("Starting HTTP server on port %d addr=%s",port)
	init_time=kernel.gettime()
	server.sock=assert(socket.tcp())
	assert(server.sock:setoption("reuseaddr",true))
	assert(server.sock:bind("*",port))
	assert(server.sock:listen(10))
	assert(server.sock:settimeout(0.1))
	server.list={}
	server.n=0
	
	while true do
		local list,err,line,clt=nil
		clt = server.sock:accept()
		if clt then
			clt:settimeout(0.1)
			table.insert(server.list, clt)
		end
		list, _, err = socket.select(server.list, nil, 0.1)
		for i, clt in ipairs(list) do
			line, err = clt:receive()
			if err then
				table.remove(server.list, i)
			else
				local req = {}
				local tmp
				local j

				tmp={string.match(line, "([^%s]+) +([^%s]+) +([^%s]+)")}
				if tmp[1] == "GET" or tmp[1] == "POST" then
					req.type = tmp[1]
				else
					clt:send(stdresp("405 Method not allowed"))
					table.remove(server.list, i)
					clt:close()
					break
				end				
				--print("METHOD",req.type)
				if tmp[2] ~= nil then
					req.file = string.gsub(tmp[2], "^/", "")
				end
				
				req.header = {}
				while 1 do
					line, err = clt:receive()
					if err then
						if req.type == "POST" and line then
							req.post = line
						end
						break
					end
					if line == nil then break end
					tmp = {string.match(line,"([^:]+)%s*:%s*(.+)")}
					if tmp[1] ~= nil and tmp[2] ~= nil then
						req.header[tmp[1]] = tmp[2]
					end
				end
				
				repeat
				if auth then
				   if req.header.Authorization~="Basic "..auth then
				   	local data="<html>Authorization Required</html>"
						local resp=stdresp("401 Not Authorized")..
				 						"Content-Type: text/html\r\n"..
				 						"Content-Length: "..(#data).."\r\n\r\n"..
				 						data
						clt:send(resp)
						break
				   end
				end
				if req.type == "GET" then
					if req.file == "" then
						local data=get_index()
						local resp=stdresp("200 OK")..
				 						"Content-Type: text/html\r\n"..
				 						"Content-Length: "..(#data).."\r\n\r\n"..
				 						data
				 		send(clt,resp)
					elseif req.file == "stats" then
						local data=get_stats()
						local resp=stdresp("200 OK")..
				 						"Content-Type: application/json\r\n"..
				 						"Content-Length: "..(#data).."\r\n\r\n"..
				 						data
				 		send(clt,resp)
					elseif req.file == "leda.css" then
						local data=get_css()
						local resp=stdresp("200 OK")..
				 						"Content-Type: text/css\r\n"..
				 						"Content-Length: "..(#data).."\r\n\r\n"..
				 						data
				 		send(clt,resp)
					elseif req.file == "smaps.json" then
						local data=get_smaps()
						local resp=stdresp("200 OK")..
				 						"Content-Type: application/json\r\n"..
				 						"Content-Length: "..(#data).."\r\n\r\n"..
				 						data
				 		send(clt,resp)
					elseif req.file == "jquery.js" then
						local data=get_jquery()
						local resp=stdresp("200 OK")..
				 						"Content-Type: text/javascript\r\n"..
				 						"Content-Length: "..(#data).."\r\n\r\n"..
				 						data
				 		send(clt,resp)
					elseif req.file == "jquery.flot.js" then
						local data=get_jquery_flot()
						local resp=stdresp("200 OK")..
				 						"Content-Type: text/javascript\r\n"..
				 						"Content-Length: "..(#data).."\r\n\r\n"..
				 						data
				 		send(clt,resp)
					elseif req.file == "jquery.arbor.js" then
						local data=get_jquery_arbor()
						local resp=stdresp("200 OK")..
				 						"Content-Type: text/javascript\r\n"..
				 						"Content-Length: "..(#data).."\r\n\r\n"..
				 						data
				 		send(clt,resp)
					elseif req.file == "jquery.flot.navigate.js" then
						local data=get_jquery_flot_navigate()
						local resp=stdresp("200 OK")..
				 						"Content-Type: text/javascript\r\n"..
				 						"Content-Length: "..(#data).."\r\n\r\n"..
				 						data
				 		send(clt,resp)
					else
						print("FILE 404",req.file)
						local data="<html>Not Found</html>"
						local resp=stdresp("404 Not Found")..
				 						"Content-Type: text/html\r\n"..
				 						"Content-Length: "..(#data).."\r\n\r\n"..
				 						data
				 		send(clt,resp)
					end
				elseif req.type == "POST" then
					print('POST',req.post)
				end
						
				if req.header.Connection == "close" then
					table.remove(server.list, i)
					clt:close()
				end
			until true
			end
		end
	end
end

local function get_init(n,port,user,pass)
	assert(n,"Number of threads required")
   if user then
      assert(pass,"Password must be provided")
      auth=mime.b64(user..":"..pass)
   end
   return   function(g)
               graph=g
               pool_size=n
               for i=1,n do
                  table.insert(th,kernel.thread_new())
                  dbg("Thread %d created",i)
               end
               http_server(port or default_port)
            end
end

t.init=get_init(default_thread_pool_size)

function t.finish()
   for i=1,#th do
      th[i]:kill()
   end
   dbg "Controller finished"
end

function t.get(...)
   return {init=get_init(...),finish=t.finish}
end

if leda and leda.controller then
   leda.controller.thread_pool=t
end

get_css=function()
	return require 'leda.controller.http_css'
end



get_jquery=function()
	return require 'leda.controller.http_jq'
end

get_jquery_flot=function()
	return require 'leda.controller.http_flot' 
end

get_jquery_arbor=function()
	return require 'leda.controller.http_arbor' 
end

get_jquery_flot_navigate=function()
	return require 'leda.controller.http_flot_navigate' 
end


get_index=function()
	return require 'leda.controller.http_index'
end

return t
