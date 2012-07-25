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
local default_thread_pool_size=1

module("leda.controller.fixed_thread_pool")

-----------------------------------------------------------------------------
-- Controller init function
-----------------------------------------------------------------------------

function get_init(n)
   return   function()
               for i=1,n do
                  kernel.new_thread()
                  dbg("Thread %d created",i)
               end
            end
end
init=get_init(default_thread_pool_size)

function wait_condition()
   dbg("Waiting queue size to become zero: %d",kernel.ready_queue_size())
   kernel.sleep(0.1)
--   while(kernel.ready_queue_size() ~= 0) do
   while true do
--      dbg("queue size is %d",kernel.ready_queue_size())
      kernel.sleep(10000)
   end
--   dbg("queue size is '%d'",kernel.ready_queue_size())
end

function get(n)
   return {init=get_init(n),wait_condition=wait_condition}
end
