-----------------------------------------------------------------------------
-- Leda simple single threaded controller
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

-----------------------------------------------------------------------------
-- Declare module and import dependencies
-----------------------------------------------------------------------------
local base = _G
local dbg = leda.debug.get_debug("Controller: Sigle-thread: ")
local kernel=leda.kernel
module("leda.controller.singlethread")

-----------------------------------------------------------------------------
-- Controller init function
-----------------------------------------------------------------------------
function init()
   kernel.new_thread()
   dbg("Controller created")
end

function wait_condition()
   while(kernel.ready_queue_size() > 0) do
      dbg(kernel.ready_queue_size())
      kernel.sleep(1)
   end
end
