-----------------------------------------------------------------------------
-- Leda default controler
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

-----------------------------------------------------------------------------
-- Declare module and import dependencies
-----------------------------------------------------------------------------
local interactive=require("leda.controller.interactive")

--module("leda.controller.default")
local t={}

-----------------------------------------------------------------------------
-- Controller init function
-----------------------------------------------------------------------------

t.init=interactive.init
return t
