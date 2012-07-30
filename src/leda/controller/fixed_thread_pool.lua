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
local table=table
local default_thread_pool_size=10
local print=print

module("leda.controller.fixed_thread_pool")

local pool_size=default_thread_pool_size
local th={}
-----------------------------------------------------------------------------
-- Controller init function
-----------------------------------------------------------------------------

function get_init(n)
   return   function()
               pool_size=n
               for i=1,n do
                  table.insert(th,kernel.new_thread())
                  dbg("Thread %d created",i)
               end
            end
end
init=get_init(default_thread_pool_size)

function event_pushed(timedout)
   --print("Write happened",timedout)
   local ps=kernel.thread_pool_size()
   local qs=kernel.ready_queue_size()
end

function get(n)
   return {init=get_init(n),event_pushed=event_pushed,finish=finish}
end

function finish()
   for i=1,#th do
      th[i]:kill()
   end
   dbg "Controller finished"
end
