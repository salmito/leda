-----------------------------------------------------------------------------
-- Leda simple single threaded controller
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

-----------------------------------------------------------------------------
-- Declare module and import dependencies
-----------------------------------------------------------------------------
local fixed_thread_pool=require("leda.controller.fixed_thread_pool")
local singlethread=fixed_thread_pool.get(1)

--module("leda.controller.singlethread")

-----------------------------------------------------------------------------
-- Controller init function
-----------------------------------------------------------------------------
leda.controller=leda.controller or {}
leda.controller.singlethread=singlethread

return singlethread
