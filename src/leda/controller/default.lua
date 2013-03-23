-----------------------------------------------------------------------------
-- Leda default controler
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

local interactive=require("leda.controller.interactive")

--module("leda.controller.default")
local t=interactive.get(leda.kernel.cpu())

if leda and leda.controller then
   leda.controller.default=t
end

return t
