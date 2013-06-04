-----------------------------------------------------------------------------
-- Leda default controler
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

local controller=require("leda.controller.thread_pool")

--module("leda.controller.default")
local t=controller

if leda and leda.controller then
   leda.controller.default=t
end

return t
