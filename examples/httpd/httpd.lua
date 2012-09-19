require "httpd_impl"

local leda=require "leda"
local interactive=require "leda.controller.interactive"

local port=port or 8080

local s_wait_client=leda.stage{
	name="Wait client",
	handler=wait_client,
	bind=wait_client_bind,
	init=require_socket,
}

local s_handle_connection=leda.stage{
	name="Connection handler",
	handler=handle_connection,
	bind=handle_connection_bind,
	init=require_socket,
}

local s_handle_request=leda.stage{
	name="Request handler",
	handler=handle_request,
	bind=handle_request_bind,
	init=function () require "leda.utils.socket" require "string" end,
}

local webroot=webroot or './web'

local s_cgi_handler=leda.stage{
	name="CGILua Handler",
	handler=cgilua_handler,
	init=get_cgilua_handler_init(webroot),
   bind=cgi_handler_bind,
}

local s_file_handler=leda.stage{
	name="File Handler",
	handler=file_handler,
	bind=file_handler_bind,
	init=get_file_handler_init(webroot),
}

local s_connection_close=leda.stage{
	name="Close connection",
	handler=close_connection,
	bind=close_connection_bind,
	init=require_socket,
}


s_wait_client:send(port)

local webserver=leda.graph{name="Webserver"}
webserver:add(leda.connect(s_wait_client,'connection',s_handle_connection,leda.couple))
--webserver:add(leda.connect(s_send_response,'send_data',s_send_data))
webserver:add(leda.connect(s_handle_connection,'handle_request',s_handle_request,leda.couple))
webserver:add(leda.connect(s_handle_request,'file_handler',s_file_handler,leda.couple))
webserver:add(leda.connect(s_handle_request,'cgilua_handler',s_cgi_handler,leda.couple))
webserver:add(leda.connect(s_cgi_handler,'close_connection',s_connection_close,leda.couple))
--leda.connect(s_file_handler,'send_response',s_send_response))
webserver:add(leda.connect(s_file_handler,'close_connection',s_connection_close,leda.couple))
--leda.connect(s_send_data,'close_connection',s_connection_close))
webserver:add(leda.connect(s_connection_close,'connection',s_handle_connection,leda.couple))


--leda.plot_graph(webserver)

webserver:plot('graph.png')

webserver:run()
