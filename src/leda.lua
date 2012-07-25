-----------------------------------------------------------------------------
-- Leda Lua API
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

local debug=require("leda.debug")
local kernel = require("leda.kernel")
local l_connector = require("leda.l_connector")
local l_stage = require("leda.l_stage")
local l_graph = require("leda.l_graph")

local dbg = leda.debug.get_debug("Leda: ")

module("leda")

--Exporting graph related functions
graph=l_graph.graph
is_graph=l_graph.is_graph
dump_graph=l_graph.dump

--Exporting stage related functions
stage=l_stage.new_stage
is_stage=l_stage.is_stage

--Exporting built in connector types tunctions
pass_thread=l_connector.call_func
throw_event=l_connector.emmit_func
pass_thread_and_throw_event=l_connector.emmit_self_call_func

--Short name for them
t=pass_thread
e=throw_event
te=pass_thread_and_throw_event

--Exporting connector related functions
connector=l_connector.new_connector
is_connector=l_connector.is_connector
