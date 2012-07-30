-----------------------------------------------------------------------------
-- Leda default controler
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

-----------------------------------------------------------------------------
-- Declare module and import dependencies
-----------------------------------------------------------------------------
local fixed_thread_pool=require("leda.controller.fixed_thread_pool")

module("leda.controller.default")

-----------------------------------------------------------------------------
-- Controller init function
-----------------------------------------------------------------------------

init=fixed_thread_pool.init
event_pushed=fixed_thread_pool.event_pushed
finish=fixed_thread_pool.finish
