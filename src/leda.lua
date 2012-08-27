-----------------------------------------------------------------------------
-- Leda Lua API
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

local kernel = require("leda.kernel")
local debug= require("leda.debug")
local leda_stage = require("leda.leda_stage")
local leda_graph = require("leda.leda_graph")
local daemon = require("leda.daemon")

module("leda")

--Exporting graph related functions
graph=leda_graph.graph
is_graph=leda_graph.is_graph
is_cluster=leda_graph.is_cluster
dump_graph=leda_graph.dump

emmit=leda_connector.emmit
call=leda_connector.call
fork=leda_connector.fork

--Exporting stage related functions
stage=leda_stage.new_stage
is_stage=leda_stage.is_stage

--Exporting connector related functions
is_connector=leda_connector.is_connector
start_daemon=daemon.start
