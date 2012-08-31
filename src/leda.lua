-----------------------------------------------------------------------------
-- Leda Lua API
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

local kernel = require("leda.kernel")
local debug= require("leda.debug")
local leda_stage = require("leda.leda_stage")
local leda_cluster = require("leda.leda_cluster")
local leda_graph = require("leda.leda_graph")
local process = require("leda.process")

module("leda")

--Exporting graph related functions
graph=leda_graph.graph
is_graph=leda_graph.is_graph
is_cluster=leda_graph.is_cluster
dump_graph=leda_graph.dump

couple=leda_connector.coupled
cohort=leda_connector.cohort

--Exporting stage related functions
stage=leda_stage.new_stage
is_stage=leda_stage.is_stage

--Exporting connector related functions
is_connector=leda_connector.is_connector
start=process.start

--Exporting cluster related functions
cluster=leda_cluster.new_cluster
