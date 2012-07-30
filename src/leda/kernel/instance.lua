----------------------------------------------------------------------------
-- Leda's Instance Internal Lua API
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

-----------------------------------------------------------------------------
-- Build proxies for the output of the stage
-----------------------------------------------------------------------------
leda={output={}}

-----------------------------------------------------------------------------
-- Define an easier name for the new_mutex method
-----------------------------------------------------------------------------
leda.mutex=__mutex

-----------------------------------------------------------------------------
-- Define an easier name for the wait_event method
-----------------------------------------------------------------------------
leda.wait_event=__wait_event

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
   return nil,string.format("Output '%s' not found",key)
end

-----------------------------------------------------------------------------
-- Build proxies for the output of the stage
-----------------------------------------------------------------------------
for key,connector in pairs(stage.__output) do
   local c,err={}
   --load the sendf function of the connector with 'key'
   c.sendf,err=loadstring(connector.__sendf)
   if not c.sendf then 
      error(string.format(
            "Error loading send function for stage '%s': %s",stage.__name,err))
   end
   assert(type(c.sendf)=="function",string.format("Send field must be a function (got '%s')",type(c.sendf)))
   c.consumers=connector.__consumers;
   c.send=function(self,...) self.sendf(self.consumers,...) end
   leda.output[key]=c
end

-----------------------------------------------------------------------------
-- Load the stage init function
-----------------------------------------------------------------------------
local init,err=loadstring(stage.__init)
if not init then 
   error(string.format("Error loading init function for stage '%s': %s",stage.__name,err))
else
   -- Execute init function of the stage
   init() 
end

-----------------------------------------------------------------------------
-- Load handler function of the stage with an environment of its own
-----------------------------------------------------------------------------
--local function handler_str() return stage.__handler end
local __handler,err=loadstring(stage.__handler)
if not __handler then 
   error(string.format("Error loading handler function for stage '%s': %s",stage.__name,err))
end
-----------------------------------------------------------------------------
-- Create the main coroutine for the stage handler
-----------------------------------------------------------------------------
local function main_coroutine()
   local end_code=__end_code
   while true do
      --clean environment
      if setfenv then 
         local env=setmetatable({},{__index=_G})
         setfenv(__handler,env) 
      end
      __handler(coroutine.yield(end_code)) 
   end 
end

handler=coroutine.wrap(main_coroutine)

local status=handler()
assert(status==__end_code,"Unexpected error")


