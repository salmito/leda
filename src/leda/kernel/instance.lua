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
for key,connector in pairs(stage.__output) do
   local c,err={}
   --load the sendf function of the connector with 'key'
   c.sendf,err=loadstring(connector.__sendf)
   if not c.sendf then 
      error("Error loading sendf function for stage")
   end
   assert(type(c.sendf)=="function","Sendf field must be a function")
   c.consumers=connector.__consumers;
   c.send=function(self,...) return self.sendf(self.consumers,...) end
   leda.output[key]=c
end

-----------------------------------------------------------------------------
-- Load the stage init function
-----------------------------------------------------------------------------
local init,err=loadstring(stage.__init)
if not init then 
   error("Error loading init function for stage")
else
   -- Execute init function of the stage
   pcall(init) 
end
-----------------------------------------------------------------------------
-- Load handler function of the stage 
-----------------------------------------------------------------------------
--local function handler_str() return stage.__handler end
local __handler=leda.decode(stage.__handler)
if not __handler then 
   error("Error loading handler function for stage")
end
-----------------------------------------------------------------------------
-- Create the main coroutine for the stage handler
-----------------------------------------------------------------------------
local stage=stage
local coroutine=coroutine
local function main_coroutine()
   local end_code=__end_code
   while true do
      --clean environment --DISABLED on lua 5.2
      if setfenv and not stage.__serial then 
         local env=setmetatable({},{__index=_G})
         setfenv(__handler,env)
      end
      __handler(coroutine.yield(end_code))
   end 
end

handler=coroutine.wrap(main_coroutine)

local status=handler()
assert(status==__end_code,"Unexpected error")
