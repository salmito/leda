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

if leda and leda.controller then
   leda.controller.singlethread=singlethread
end
return singlethread
