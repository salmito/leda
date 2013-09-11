local _=require 'leda'

local function sink(resp)
	local f=ltn12.sink.table(resp)
	return function(...)
--		leda.nice()
		return f(...)
	end
end

--Fetch a https url.
--Requires luasec.
local function get_https(url)
	local http = require("ssl.https")
	local resp = {}
	local r, c, h, s=http.request{
   	url = url,
   	sink = sink(resp),
   	protocol = "tlsv1"
	}
	if not r then return nil,c end
	return table.concat(resp),r,c,h,s
end

--Fetch a http url
local function get_http(url)
	local http = require("socket.http")
	local resp = {}
	local r, c, h, s=http.request{
   	url = url,
   	sink = sink(resp),
	}
	if not r then return nil,c end
	return table.concat(resp),r,c,h,s
end

local stage={}

function stage.handler(url)
	local res,err=nil
	if string.sub(url,1,8)=='https://' then
		res,err=leda.push(get_https(url))
	elseif string.sub(url,1,7)=='http://' then
		res,err=leda.push(get_http(url))
	else
	   error("Not a 'http' or 'https' url: "..tostring(url))
	end
	if not res then
		leda.send('error',err)
	end
end

function stage.init()
	require("socket")
	require("table")
end

function stage.bind(self,out,graph)
	assert(out[1],"Default port must be connected for stage "..tostring(self))
end

stage.serial=false

stage.name="HTTP Client"

return _.stage(stage)
