local _=require 'leda'
local stage={}

stage.handler=function()
    local server_sock,err=assert(socket.bind("*",self.port))
    local cli_sock=true
    while cli_sock and server_sock do
       cli_sock,err=assert(server_sock:accept())
       if cli_sock then
	       cli_sock:setoption ("tcp-nodelay", true)
   	    local cli, cport = cli_sock:getpeername()
   	    leda.push(cli_sock,cli,cport)
       end
   end
   server_sock:close()
   leda.send("error",err,self.port)
end

stage.init=function ()
	require 'leda.utils.socket'
end

function stage:bind(output)
	assert(output[1],"Default port must be connected for stage '"..tostring(self).."'")
	assert(self.port,"Field 'port' must be defined for stage '"..tostring(self).."'")
	self.port=assert(tonumber(self.port),"Field 'port' must be a number")
	self.name=self.name.." (port="..self.port..")"
end

stage.autostart=true
stage.name="TCP client"

return _.stage(stage)
