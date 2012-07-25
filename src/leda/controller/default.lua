-----------------------------------------------------------------------------
-- Leda default controler
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

-----------------------------------------------------------------------------
-- Declare module and import dependencies
-----------------------------------------------------------------------------
local base = _G
local fixed_thread_pool=require("leda.controller.fixed_thread_pool")

module("leda.controller.default")

-----------------------------------------------------------------------------
-- Controller init function
-----------------------------------------------------------------------------
init=fixed_thread_pool.init
wait_condition=fixed_thread_pool.wait_condition
