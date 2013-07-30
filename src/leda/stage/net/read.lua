local _=require 'leda'
local stage={}

stage.handler=function(sock,...)
	local data,err=true
    while data do
       data,err=sock:receive()
       if data then
	       leda.send("data",data,...)
	    end
   end
   leda.send("error",err,sock,...)
end

stage.init=function ()
	require 'leda.utils.socket'
end

function stage:bind(output)
	assert(output.data,"Port 'data' must be connected for stage '"..tostring(self).."'")
end

stage.name="Socket read"

return _.stage(stage)
