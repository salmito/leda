local leda=leda
local stage=leda.stage
local __yield_code_l=leda.__yield_code leda.__yield_code=nil
local __end_code_l=leda.__end_code leda.__end_code=nil
leda.nice=function (...) return coroutine.yield(__yield_code_l,...) end
function leda.get_output(key)
key = key or 1
if leda.output[key] then 
return leda.output[key]
end
return nil,"Output key not found"
end
function leda.send(key,...)
if not leda.output[key] then return nil, "Output key not found: "..tostring(key) end
return leda.output[key].send(...)
end
local __cohort_f=__cohort __cohort=nil
local __emmit_f=__emmit __emmit=nil
for key,connector in pairs(stage.__output) do
local c,err={}
local func=nil
if connector.__sendf=="cohort" then 
func=__cohort_f
else
func=__emmit_f
end
assert(type(func)=="function","Sendf field must be a function")
local consumer=connector.__consumer;
local id=connector.__id;
c.send=function(...) return func(consumer,id,...) end
leda.output[key]=c
end
local stage_env=leda.decode(leda.stage.__env)
local global_env=_G
setmetatable(stage_env,{__index=global_env})
leda.stage.__handler=stage_env.handler stage_env.handler=nil
leda.stage.__init=stage_env.init stage_env.init=nil
local __handler=leda.decode(leda.stage.__handler)
if not (type(__handler)=='function' or type(__handler)=='string') then 
error("Error loading handler function for stage: "..tostring(leda.stage.name).." type:"..type(__handler))
end
if type(__handler)=="string" then
__handler,err=loadstring(__handler)
if not __handler then 
   error("Error loading handler function for stage "..tostring(leda.stage.name).."': "..tostring(err))
	end
end
leda.stage.handler=__handler
if leda.stage.__init and leda.stage.__init~="" then
	local init,err=leda.decode(leda.stage.__init)
	if init then
		if type(init)=="string" then
			init,err=loadstring(init)
			if not init then 
			   error("Error loading init function for stage "..tostring(leda.stage.name).."': "..tostring(err))
	   	end
	   elseif not setfenv and type(init)=="function" then
	      local debug=require 'debug'
         local upname,old_env = debug.getupvalue (init, 1)
         if upname == '_ENV' then
           local debug=require 'debug'
           debug.setupvalue (init, 1,_ENV)
         end
      end
      
		if setfenv then       
			setfenv(init,stage_env)
		else
		local upname,old_env = debug.getupvalue (init, 1)
			if upname == '_ENV' then
		      debug.setupvalue (init, 1,stage_env)
		   end
		end

	   local ok,err=init() 
	end
end
__handler=leda.stage.handler
local debug=nil
if not setfenv then
   debug=require('debug')
end
local coroutine=coroutine
local env=setmetatable({},{__index=global_env})
self=stage_env
 --	setfenv(0,stage_env)
if setfenv then       
	setfenv(stage.handler,env)
else
local upname,old_env = debug.getupvalue (stage.handler, 1)
	if upname == '_ENV' then
      debug.setupvalue (stage.handler, 1,env)
   end
end
local function main_coroutine()
   while true do
      stage.handler(coroutine.yield(__end_code_l))
		if not stage.serial then
         env=setmetatable({},{__index=stage_env})
         --clean environment --DISABLED on lua 5.2
         if setfenv then       
            setfenv(stage.handler,env)
         else
            local upname,old_env = debug.getupvalue (stage.handler, 1)
            if upname == '_ENV' then
              debug.setupvalue (stage.handler, 1,env)
            end
         end
   	end
   end 
end
handler=coroutine.wrap(main_coroutine)
local status=handler()
assert(status==__end_code_l,"Unexpected error")
