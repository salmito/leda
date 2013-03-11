-----------------------------------------------------------------------------
-- Leda simple fixed thread pool controller
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

-----------------------------------------------------------------------------
-- Declare module and import dependencies
-----------------------------------------------------------------------------
local base = _G
local dbg = leda.debug.get_debug("Controller: Fixed-thread: ")
local kernel=leda.kernel
local table,ipairs,pairs,print=table,ipairs,pairs,print
local default_thread_pool_size=10


--module("leda.controller.fixed_thread_pool")

local t={}

local pool_size=default_thread_pool_size
local th={}

-----------------------------------------------------------------------------
-- Controller init function
-----------------------------------------------------------------------------

local function get_init(n)
   return   function()
               pool_size=n
               for i=1,n do
                  table.insert(th,kernel.thread_new())
                  dbg("Thread %d created",i)
               end
            end
end
t.init=get_init(default_thread_pool_size)

--[[function event_pushed(timedout,stats)
   local ps=kernel.thread_pool_size()
   local rs=kernel.ready_queue_size()
   local rc=kernel.ready_queue_capacity()
   print("Write happened",timedout,ps,rs,rc)
   if stats then
   for k,v in ipairs(stats) do 
      print(k)
      for k2,v2 in pairs(v) do
         print(k2,v2)
      end
   end

   end
end--]]

function t.get(n)
   return {init=get_init(n),event_pushed=event_pushed,finish=finish}
end

function t.finish()
   for i=1,#th do
      th[i]:kill()
   end
   dbg "Controller finished"
end

return t
