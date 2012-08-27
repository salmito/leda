require "httpd_impl"

local leda=require "leda"
require "leda.utils.plot"
local interactive=require "leda.controller.interactive"

local port=port or 8080

local s_wait_client=leda.stage{
	"Wait client",
	handler=wait_client,
	bind=function(self) assert(self.output.connection,"Output 'connection' field must be connected") end,
	init=require_socket,
}

local s_handle_connection=leda.stage{
	"Connection handler",
	handler=handle_connection,
	bind=function(self) assert(self.output.handle_request,"Output 'handle_request' field must be connected") end,
	init=require_socket,
}

local s_handle_request=leda.stage{
	"Request handler",
	handler=handle_request,
	bind=function(self) assert(self.output.file_handler,"Output 'send_response' field must be connected") 
                            assert(self.output.cgilua_handler,"Output 'send_data' field must be connected") end,
	init=function () require "string" end,
}

local webroot=webroot or './web'

local s_cgi_handler=leda.stage{
	"CGILua Handler",
	handler=cgilua_handler,
	init=get_cgilua_handler_init(webroot),
   bind=function(self) assert(self.output.send_data,"Output 'send_data' field must be connected") end,
}

local s_file_handler=leda.stage{
	"File Handler",
	handler=file_handler,
	bind=function(self) assert(self.output.send_response,"Output 'send_response' field must be connected") 
                            assert(self.output.send_data,"Output 'send_data' field must be connected") end,
	init=get_file_handler_init(webroot),
}

local s_send_response=leda.stage{
	"Send response",
	handler=send_response,
	bind=function(self) assert(self.output.send_data,"Output 'send_data' field must be connected") end,
	init=require_socket,
}

local s_send_data=leda.stage{
	"Send data chunk",
	handler=send_data,
	bind=function(self) assert(self.output.close_connection,"Output 'close_connection' field must be connected") end,
	init=require_socket,
}

local s_connection_close=leda.stage{
	"Close connection",
	handler=close_connection,
	bind=function(self) assert(self.output.connection,"Output 'connection' field must be connected") end,
	init=require_socket,
}

leda.connect(s_wait_client,'connection',s_handle_connection)
leda.connect(s_send_response,'send_data',s_send_data)
leda.connect(s_handle_connection,'handle_request',s_handle_request)
leda.connect(s_handle_request,'file_handler',s_file_handler)
leda.connect(s_handle_request,'cgilua_handler',s_cgi_handler)
leda.connect(s_cgi_handler,'send_data',s_send_data)
leda.connect(s_file_handler,'send_response',s_send_response)
leda.connect(s_file_handler,'send_data',s_send_data)
leda.connect(s_send_data,'close_connection',s_connection_close)
leda.connect(s_connection_close,'connection',s_handle_connection)

s_send_data:input_method(leda.t)

s_wait_client:send(port)

local webserver=leda.graph{"Webserver",
   s_wait_client,
   s_handle_connection,
   s_handle_request,
   s_cgi_handler,
   s_file_handler,
   s_send_response,
   s_send_data,
   s_connection_close
}

leda.plot_graph(webserver)

webserver:run(interactive.get(10))
