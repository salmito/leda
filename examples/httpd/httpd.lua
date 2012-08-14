-----------------------------------------------------------------------------
-- Xavante HTTP handler
--
-- Authors: Javier Guerra and Andre Carregal
-- Copyright (c) 2004-2007 Kepler Project
--
-- $Id: httpd.lua,v 1.45 2009/08/10 20:00:59 mascarenhas Exp $
-----------------------------------------------------------------------------

--modified for leda

require "leda"
require "leda.utils.plot"

-- Manages one connection, maybe several requests
-- params:
--		skt : client socket

wait_client=stage{
   handler=function(port)
       local server_sock=socket.bind("*",port)
       print("SERVER: Waiting on port >> ",port)
       while true do
          local cli_sock=server_sock:accept()
          
          cli_sock:setoption ("tcp-nodelay", true)
          local cli, port = cli_skt:getsockname ()
          local cli=leda.socket.wrap(cli_sock)
          leda.send("read-method",cli)
       end
   end, 
   init=function () 
      require "socket"
   end,
   name="wait client"
}


	--skt:setoption ("tcp-nodelay", true)
	--local srv, port = skt:getsockname ()
	local req = {
--		rawskt = skt,
		srv = srv,
		port = port,
--		copasskt = copas.wrap (skt),
--      ledaskt = leda.socket.wrap (skt)
	}
	req.socket = req.copasskt
	req.serversoftware = _serversoftware

local connection=leda.stage{"connection",
handler=function (skt,serversoftware)
   local cli_skt=leda.socket.unwrap(skt)
	
	local cli, port = cli_skt:getsockname ()
	local req = {
--		rawskt = skt,
		srv = srv,
		port = port,
      ledaskt = leda.socket.wrap (cli_skt)
	}
	req.serversoftware = serversoftware or ""
	
	leda.send("read_method")
end,
bind=function(s)
  -- assert(s.output.read_method,"read_method must be connected")
end,
   init=function () require "socket" end,
}

-- gets and parses the request line
-- params:
--		req: request object
-- returns:
--		true if ok
--		false if connection closed
-- sets:
--		req.cmd_mth: http method
--		req.cmd_url: url requested (as sent by the client)
--		req.cmd_version: http version (usually 'HTTP/1.1')
local read_method=leda.stage{"read_method",
handler=function (req_enc)
   local req=leda.decode(req_enc)
   local skt=leda.socket.unwrap(req.ledaskt)
	local err
	req.cmdline, err = skt:receive ()
	
	if not req.cmdline then return nil end
	req.cmd_mth, req.cmd_url, req.cmd_version = unpack (strsplit (req.cmdline))
	req.cmd_mth = string.upper (req.cmd_mth or 'GET')
	req.cmd_url = req.cmd_url or '/'
	leda.socket.wrap(skt)
	leda.send("read_headers",leda.encode(req))
end,
bind=function(s)
  -- assert(s.output.read_headers,"read_headers must be connected")
end,
init=function()
require "socket" 
function strsplit (str)
	local words = {}
	
	for w in string.gmatch (str, "%S+") do
		table.insert (words, w)
	end
	
	return words
end
end
}

-- gets and parses the request header fields
-- params:
--		req: request object
-- sets:
--		req.headers: table of header fields, as name => value
local read_headers=leda.stage{"read_headers",
handler=function (req_t)
   local req=leda.decode(req_t)
   req.socket=leda.socket.unwrap(req.ledaskt)
	local headers = {}
	local prevval, prevname
	
	while 1 do
		local l,err = req.socket:receive ()
		if (not l or l == "") then
			req.headers = headers
			break
		end
		local _,_, name, value = string.find (l, "^([^: ]+)%s*:%s*(.+)")
		name = string.lower (name or '')
		if name then
			prevval = headers [name]
			if prevval then
				value = prevval .. "," .. value
			end
			headers [name] = value
			prevname = name
		elseif prevname then
			headers [prevname] = headers [prevname] .. l
		end
	end
	leda.socket.wrap(req.socket)
	req.socket=nil
	leda.send("parse_url",leda.encode(req))
end,
bind=function(s)
 --  assert(s.output.parse_url,"parse_url must be connected")
end,
   init=function () require "socket" end,
}

local parse_url=leda.stage{"parse_url",
handler=function  (req_t)
   local req=leda.decode(req_t)
	local def_url = string.format ("http://%s%s", req.headers.host or "", req.cmd_url or "")
	
	req.parsed_url = url.parse (def_url or '')
	req.parsed_url.port = req.parsed_url.port or req.port
	req.built_url = url.build (req.parsed_url)
	
	req.relpath = url.unescape (req.parsed_url.path)
	leda.send("make_response",leda.encode(req))
end,
bind=function(s)
 --  assert(s.output.make_response,"make_response must be connected")
end,
init=function ()
url = require "socket.url"
end
}

local make_response=leda.stage{"make_response",
handler=function (req_t)
   local req=leda.decode(req_t)
	local res = {
		req = leda.encode(req),
--		socket = req.socket,
		headers = default_headers (req),
--		add_header = add_res_header,
--		send_headers = send_res_headers,
--		send_data = send_res_data,
	}
	
	leda.send("send_response",leda.encode(res))
end,
bind=function(s)
 --  assert(s.output.send_response,"send_response must be connected")
end,
init=function() 
   require "socket"
-- sets the default response headers
function default_headers (req)
	return  {
		Date = os.date ("!%a, %d %b %Y %H:%M:%S GMT"),
		Server = _serversoftware,
	}
end


end
}

local send_response=leda.stage{"send_response",
handler=function(res_t)
   local res=leda.decode(res_t)
   for k,v in pairs(res) do print(k,v) end
end,
}

local read_response=leda.stage{"send_response",
handler=function(res_t)
   local res=leda.decode(res_t)
   for k,v in pairs(res) do print(k,v) end
end,
}

local send_headers=leda.stage{"send_headers",
handler=function(res_t)
   local res=leda.decode(res_t)
   for k,v in pairs(res) do print(k,v) end
end,
}


local read_file=leda.stage{"read_file",
handler=function(res_t)
   local res=leda.decode(res_t)
   for k,v in pairs(res) do print(k,v) end
end,
}

local send_block=leda.stage{"send_block",
handler=function(res_t)
   local res=leda.decode(res_t)
   for k,v in pairs(res) do print(k,v) end
end,
}

local close=leda.stage{"connection_close",
handler=function(res_t)
   local res=leda.decode(res_t)
   for k,v in pairs(res) do print(k,v) end
end,
}
read_method.output['read_headers']=read_headers.input
read_headers.output['make_response']=make_response.input
make_response.output['send_headers']=send_headers.input
send_headers.output['read_file_block']=read_file.input

read_file.output['send_block']=send_block.input
send_block.output['read_next_block']=read_file.input
close.output['keep-alive']=read_method.input

wait_client.output['read_method']=read_method.input
send_headers.output['close']=close.input
send_block.output['close']=close.input

local webserver=leda.graph{
   wait_client,
   read_method,
   read_headers,
   make_response,
   send_headers,
   read_file,
   send_block,
   close,
   }

leda.plot_graph(webserver)

wait_client:send(8080)

webserver:run()
