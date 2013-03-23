----------------------------------------------------------------------------
-- Leda's Instance Internal Lua API
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------
-----------------------------------------------------------------------------
-- Build proxies for the output of the stage
-----------------------------------------------------------------------------
leda={
   output={},
}

leda.encode=__encode
leda.decode=__decode
leda.clone=__clone

leda.io=__io
leda.epoll=__epoll
leda.aio=__aio

-----------------------------------------------------------------------------
-- Define an easier name for the utils libraries
-----------------------------------------------------------------------------
leda.stage=__stage
local stage=leda.stage
leda.mutex=__mutex
leda.gettime=__gettime
leda.getmetatable=__getmetatable;
leda.setmetatable=__setmetatable;

leda.nice=function (...) return coroutine.yield(__yield_code,...) end
-----------------------------------------------------------------------------
-- Define an easier name for the wait_event method
-----------------------------------------------------------------------------
leda.debug={
      wait_event=__wait_event,
      peek_event=__peek_event
}
--leda.wait_event=__wait_event
--leda.peek_event=__peek_event

-----------------------------------------------------------------------------
-- Define an easier name for the sleep function
-----------------------------------------------------------------------------
leda.sleep=__sleep
leda.quit=__quit

-----------------------------------------------------------------------------
-- Function to get the output indexed by 'key'
-- if 'key' is absent, return the fist field of output table
-- if 'key' is not defined, return 'nil' and an error message
-----------------------------------------------------------------------------
function leda.get_output(key)
   key = key or 1
   --try to get the provided key
   if leda.output[key] then 
      return leda.output[key]
   end
   return nil,"Output key not found"
end

function leda.send(key,...)
   if not leda.output[key] then return nil, "Output key not found: "..tostring(key) end
   return leda.output[key]:send(...)
end

-----------------------------------------------------------------------------
-- Build proxies for the output of the stage
-----------------------------------------------------------------------------
for key,connector in pairs(leda.stage.__output) do
   local c,err={}
   --load the sendf function of the connector with 'key'
--   c.sendf,err=loadstring(connector.__sendf)
   if connector.__sendf=="cohort" then 
      c.sendf=__cohort
   else
      c.sendf=__emmit
--      error("Error loading send function for stage '"..leda.stage.name.."': Unknown connector type")
   end

   assert(type(c.sendf)=="function","Sendf field must be a function")

   c.consumer=connector.__consumer;
   c.id=connector.__id;
   c.send=function(self,...) return self.sendf(c.consumer,c.id,...) end
   leda.output[key]=c
end

-----------------------------------------------------------------------------
-- Load handler function of the stage 
-----------------------------------------------------------------------------
--local function handler_str() return stage.__handler end
__handler=leda.decode(leda.stage.__handler)
if not (type(__handler)=='function' or type(__handler)=='string') then 
   error("Error loading handler function for stage: "..tostring(leda.stage.name).." type:"..type(__handler))
end

if type(__handler)=="string" then
	__handler,err=loadstring(__handler)
	if not __handler then 
	   error("Error loading handler function for stage "..tostring(leda.stage.name).."': "..tostring(err))
	end
end

-----------------------------------------------------------------------------
-- Load the stage init function
-----------------------------------------------------------------------------
if leda.stage.__init and leda.stage.__init~="" then
	local init,err=leda.decode(leda.stage.__init)
	if not init then 
	   error("Error loading init function for stage '"..leda.stage.name.."': "..err)
	else
		if type(init)=="string" then
			init,err=loadstring(init)
			if not init then 
			   error(string.format("Error loading init function for stage '%s': %s", stage.__name,err))
	   	end
		end
	   -- Execute init function of the stage
	   local ok,err=pcall(init) 
	   if not ok then error("Error executing init function for stage '"..leda.stage.name.."': ".. err)
	   end
	end
end


-----------------------------------------------------------------------------
-- Create the main coroutine for the stage handler
-----------------------------------------------------------------------------
local debug=nil
if not setfenv then
   debug=require('debug')
end

local coroutine=coroutine
local function main_coroutine()
   local end_code=__end_code
   while true do
      local env=setmetatable({},{__index=_G})
      --clean environment --DISABLED on lua 5.2
      if setfenv and not stage.serial then       
         setfenv(__handler,env)
      else
         local upname,old_env = debug.getupvalue (__handler, 1)
         if upname == '_ENV' then
           debug.setupvalue (__handler, 1,env)
         end
      end
      __handler(coroutine.yield(end_code))
   end 
end

handler=coroutine.wrap(main_coroutine)

local status=handler()
assert(status==__end_code,"Unexpected error")
