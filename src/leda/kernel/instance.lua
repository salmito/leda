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
-----------------------------------------------------------------------------
-- Define an easier name for the utils libraries
-----------------------------------------------------------------------------
leda.stage=__stage
local stage=leda.stage
leda.mutex=__mutex
leda.io=__io
leda.epoll=__epoll
leda.socket=__socket

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

-----------------------------------------------------------------------------
-- Function to get the output indexed by 'key'
-- if 'key' is absent, return the fist field of output table
-- if 'key' is not defined, return 'nil' and an error message
-----------------------------------------------------------------------------
function leda.get_output(key)
   --try to get the provided key
   if key and leda.output[key] then 
      return leda.output[key]
   end
   --if not found use the first integer key
   key=1
   if leda.output[key] then 
      return leda.output[key]
   end
   return nil,"Output key not found"
end

function leda.send(key,...)
   if not leda.get_output(key) then return nil, "Output key not found: "..tostring(key) end
   return leda.get_output(key):send(...)
end

-----------------------------------------------------------------------------
-- Build proxies for the output of the stage
-----------------------------------------------------------------------------
for key,connector in pairs(leda.stage.__output) do
   local c,err={}
   --load the sendf function of the connector with 'key'
--   c.sendf,err=loadstring(connector.__sendf)
   if connector.__sendf=="emmit" then 
      c.sendf=__emmit
   elseif connector.__sendf=="call" then 
      c.sendf=__call
   elseif connector.__sendf=="fork" then 
      c.sendf=__fork
   else
      error("Error loading send function for stage '"..leda.stage.name.."': Unknown connector type")
   end
   assert(type(c.sendf)=="function","Sendf field must be a function")
   c.consumer=connector.__consumer;
   c.send=function(self,...) return self.sendf(self.consumer,...) end
   leda.output[key]=c
end

-----------------------------------------------------------------------------
-- Load the stage init function
-----------------------------------------------------------------------------
if leda.stage.__init and leda.stage.__init~="" then
local init,err=leda.decode(leda.stage.__init)
if not init then 
   error("Error loading init function for stage '"..leda.stage.name.."': "..err)
else
   -- Execute init function of the stage
   local ok,err=pcall(init) 
   if not ok then error("Error executing init function for stage '"..leda.stage.name.."': "..err) end
end
end
-----------------------------------------------------------------------------
-- Load handler function of the stage 
-----------------------------------------------------------------------------
--local function handler_str() return stage.__handler end
local __handler=leda.decode(leda.stage.__handler)
if not __handler then 
   error("Error loading handler function for stage")
end
-----------------------------------------------------------------------------
-- Create the main coroutine for the stage handler
-----------------------------------------------------------------------------
local coroutine=coroutine
local function main_coroutine()
   local end_code=__end_code
   while true do
      --clean environment --DISABLED on lua 5.2
      if setfenv and not stage.serial then 
         local env=setmetatable({},{__index=_G})
         setfenv(__handler,env)
      end
      __handler(coroutine.yield(end_code))
   end 
end

handler=coroutine.wrap(main_coroutine)

local status=handler()
assert(status==__end_code,"Unexpected error")
