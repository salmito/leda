-----------------------------------------------------------------------------
-- Leda default controler
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

-----------------------------------------------------------------------------
-- Declare module and import dependencies
-----------------------------------------------------------------------------
local interactive=require("leda.controller.interactive")
local fixed=require("leda.controller.singlethread")

--module("leda.controller.default")
local t={}

-----------------------------------------------------------------------------
-- Controller init function
-----------------------------------------------------------------------------

t.init=fixed.init
return t
