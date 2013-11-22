-----------------------------------------------------------------------------
-- Leda simple single threaded controller
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

local thread_pool=require("leda.controller.thread_pool")
local leda=require'leda'
local singlethread=thread_pool.get(1)

if leda and leda.controller then
   leda.controller.singlethread=singlethread
end
return singlethread
