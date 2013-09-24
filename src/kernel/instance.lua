local leda=leda
local stage=leda.stage

local __yield_code_l=leda.__yield_code leda.__yield_code=nil
local __end_code_l=leda.__end_code leda.__end_code=nil

leda.nice=function (...) return coroutine.yield(__yield_code_l,...) end
leda.push=function (...) return leda.send(1,...) end

package.loaded['leda']=leda

function leda.get_output(key)
	key = key or 1
	if leda.output[key] then 
		return leda.output[key]
	end
	return nil,"Output key not defined"
end

function leda.send(key,...)
	if not leda.output[key] then return nil, "Output key not defined: "..tostring(key) end
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

self=stage_env

local global_env=_G

function leda.getenv() return global_env end

local __handler=stage_env.handler stage_env.handler=nil

local __init=stage_env.init


if not type(__handler)=='function' then 
	error("Error loading handler function for stage: "..tostring(leda.stage.name).." type:"..type(__handler))
end

if __init and type(__init)=="function" then
   __init()
   stage_env.init=nil
end

local coroutine=coroutine
local function main_coroutine()
   while true do
		__handler(coroutine.yield(__end_code_l))
   end 
end

local handler=coroutine.wrap(main_coroutine)
local status=handler()
assert(status==__end_code_l,"Unexpected error")
return handler
