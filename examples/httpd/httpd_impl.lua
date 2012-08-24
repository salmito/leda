function require_socket()
   require "socket"
   url=require "socket.url"
   require "table"
   require "string"
   require "os"
end

local function strsplit (str)
	local words = {}
	
	for w in string.gmatch (str, "%S+") do
		table.insert (words, w)
	end
	
	return words
end

local server_version="Leda Webserver 1.0"

function wait_client(port)
    local server_sock=socket.bind("*",port)
    print("SERVER: Waiting on port >> ",port)
    while true do
       local cli_sock=server_sock:accept()
       cli_sock:setoption ("tcp-nodelay", true)
       local cli, port = cli_sock:getsockname()
       local raw_cli=leda.socket.wrap(cli_sock)
       leda.send("connection",raw_cli)
   end
end

local function read_method(sock,req)
	local err
	req.cmdline, err = sock:receive ()
	
	if not req.cmdline then return nil end
	req.cmd_mth, req.cmd_url, req.cmd_version = unpack (strsplit (req.cmdline))
	req.cmd_mth = string.upper (req.cmd_mth or 'GET')
	req.cmd_url = req.cmd_url or '/'
	
	return true
end

local function parse_url (req)
	local def_url = string.format ("http://%s%s", req.headers.host or "", req.cmd_url or "")
	
	req.parsed_url = url.parse (def_url or '')
	req.parsed_url.port = req.parsed_url.port or req.port
	req.built_url = url.build (req.parsed_url)
	
	req.relpath = url.unescape (req.parsed_url.path)
end

local function read_headers (socket,req)
	local headers = {}
	local prevval, prevname
	
	while 1 do
		local l,err = socket:receive ()
		if (not l or l == "") then
			req.headers = headers
			return
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
end

local function default_headers (req)
	return  {
		Date = os.date ("!%a, %d %b %Y %H:%M:%S GMT"),
		Server = server_version,
	}
end

local function make_response (raw_socket,req)
	local res = {
		req = req,
		raw_socket = raw_socket,
		headers = default_headers (req),
	}
	
	return res
end

local function send_res_headers (res)
	if (res.sent_headers) then
		return
	end
	
--	if xavante.cookies then
		--xavante.cookies.set_res_cookies (res)
--	end
    
	res.statusline = res.statusline or "HTTP/1.1 200 OK"

	local sock=leda.socket.unwrap(res.raw_socket)

	sock:send (res.statusline.."\r\n")
	for name, value in pairs (res.headers) do
                if type(value) == "table" then
                  for _, value in ipairs(value) do
                    sock:send (string.format ("%s: %s\r\n", name, value))
                  end
                else
                    sock:send (string.format ("%s: %s\r\n", name, value))
                end
	end
	sock:send ("\r\n")
	
	res.sent_headers = true
	leda.socket.wrap(sock)
end


function handle_connection(raw_cli)
	local skt=leda.socket.unwrap(raw_cli)
	skt:setoption ("tcp-nodelay", true)
	local hostname, port = skt:getsockname()


	local req = {
		raw_socket = raw_cli,
		host = hostname,
		port= port,
	}
	if read_method (skt,req) then
		read_headers (skt,req)
		parse_url (req)
		local res = make_response (raw_cli,req)
		leda.socket.wrap(skt)
		leda.send("handle_request",res)
	else
	   skt:close()
	end
end

function handle_request(res)
   local req=res.req
   
   if req.relpath=="/" then
      req.relpath="/index.html"
   end
	local is_dynamic = string.find (req.relpath,"+*.lp$")
	is_dynamic=is_dynamic or string.find (req.relpath,"+*.lua$")
	if is_dynamic then
	   leda.send("cgilua_handler",res)
	else
   	leda.send("file_handler",res)
   end
end

function file_handler(res)
	local req=res.req
	res.send_data=send_data
	file_handler(req,res)	
	
	if res.statusline then
		leda.send("send_response",res)
	else
		leda.send("send_data",res,"")
	end
end

function get_file_handler_init(webroot_p)
	return function()
		xavante={}
		require "io"
		require "os"
		require "xavante.filehandler"
		require "xavante.httpd"
		file_handler=xavante.filehandler(webroot_p)
		webroot=webroot_p
		send_data=function(res,data) return leda.send("send_data",res,data) end
	end
end

local function add_res_header (res, h, v)
    if string.lower(h) == "status" then
        res.statusline = "HTTP/1.1 "..v
    else
        local prevval = res.headers [h]
        if (prevval  == nil) then
            res.headers[h] = v
        elseif type (prevval) == "table" then
            table.insert (prevval, v)
        else
            res.headers[h] = {prevval, v}
        end
    end
end

function cgilua_handler(res)
	local req=res.req
   req.rawskt={}
   req.socket=""
   res.add_header=add_res_header
	res.send_headers=send_res_headers
	res.send_data=send_data
   req.rawskt.getpeername=function () return "" end
	cgi_handler(req,res)
   leda.send("send_data",res,"")
end

function get_cgilua_handler_init(webroot_p)
	return function()
		xavante={}
		require "io"
		require "os"
   	require "debug"
   	require "socket"
		require "xavante.cgiluahandler"
		require "xavante.httpd"
		cgi_handler=xavante.cgiluahandler.makeHandler(webroot_p)
		webroot=webroot_p
		send_data=function(res,data) return leda.send("send_data",res,data) end
	end
end


function send_response (res)
	local req=res.req

	if res.content then
		if not res.sent_headers then
			if (type (res.content) == "table" and not res.chunked) then
				res.content = table.concat (res.content)
			end
			if type (res.content) == "string" then
				res.headers["Content-Length"] = string.len (res.content)
			end
		end
	else
		if not res.sent_headers then
			res.statusline = "HTTP/1.1 204 No Content"
			res.headers["Content-Length"] = 0
		end
	end
	
    if res.chunked then
        res:add_header ("Transfer-Encoding", "chunked")
    end
    
	if res.chunked or ((res.headers ["Content-Length"]) and req.headers ["connection"] == "Keep-Alive")
	then
		res.headers ["Connection"] = "Keep-Alive"
		res.keep_alive = true
	else
		res.keep_alive = nil
	end
	
	if res.content then
		if type (res.content) == "table" then
			for _,v in ipairs (res.content) do send_res_data (res,v) end
		else
			send_res_headers (res)
			leda.send("send_data",res,res.content)
		end
	else
		send_res_headers(res)
	end
	
	if res.chunked then
		local sock=leda.socket.unwrap(res.raw_socket)
		sock:send ("0\r\n\r\n")
		sock:close()
	end
end

function send_data (res, data)
--   print("sending",res.statusline)
	if not data or data == "" then
		leda.send("close_connection",res)
		return
	end

	local sock=leda.socket.unwrap(res.raw_socket)

	if data then
		if res.chunked then
			sock:send (string.format ("%X\r\n", string.len (data)))
			sock:send (data)
			sock:send ("\r\n")
		else
			sock:send (data)
		end
	end
	--sock:close()
	leda.socket.wrap(sock)
end

function close_connection(res)
--   print("closing",res.headers["Connection"],res.keep_alive)
   if res.keep_alive then
      leda.send("connection",res.raw_socket)
   else
      local s=leda.socket.unwrap(res.raw_socket)
      s:close()
   end
end

return _G
