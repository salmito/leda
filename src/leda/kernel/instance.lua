----------------------------------------------------------------------------
-- Leda's Instance Internal Lua API
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

-----------------------------------------------------------------------------
-- Build proxies for the output of the stage
-----------------------------------------------------------------------------
leda={output={}}

-----------------------------------------------------------------------------
-- Function to get the output indexed by 'key'
-- if 'key' is ascent, return hole output table
-- if 'key' is not defined, return 'nil' and an error message
-----------------------------------------------------------------------------
function leda.get_output(key)
   if key then
      if leda.output[key] then 
         return leda.output[key]
      end
      return nil,"Output 'key' not found";
   end
   return leda.output
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
   -----------------------------------------------------------------------------
   -- Execute init function of the stage
   -----------------------------------------------------------------------------
   init() 
end

-----------------------------------------------------------------------------
-- Load handler function of the stage
-----------------------------------------------------------------------------
local __handler,err=loadstring(stage.__handler);
if not __handler then 
   error(string.format("Error loading handler function for stage '%s': %s",stage.__name,err))
end

-----------------------------------------------------------------------------
-- Create a new environment for the stage
-----------------------------------------------------------------------------
local function f()
   while true do
      __handler(coroutine.yield(0)) 
   end 
end

handler=coroutine.wrap(f)
local status=handler()
assert(status==0,"Unexpected error")


