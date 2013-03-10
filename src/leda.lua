-----------------------------------------------------------------------------
-- Leda Main API
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

local t={}

--Define main global variable leda with methods
leda=t

t.kernel=require("leda.kernel")
t.debug= require("leda.debug")


local leda_connector = require("leda.leda_connector")
--Exporting connector related functions
t.is_connector=leda_connector.is_connector
local leda_stage = require("leda.leda_stage")
local leda_cluster = require("leda.leda_cluster")
local leda_graph = require("leda.leda_graph")
local process = require("leda.process")
t.leda_stage=leda_stage
t.leda_cluster=leda_cluster
t.leda_graph=leda_graph
t.connect=leda_stage.connect
t.process=process
t.controller={}

local print,pairs=print,pairs



--module("leda")
t.gettime=kernel.gettime

--Exporting graph related functions
t.graph=leda_graph.graph
t.is_graph=leda_graph.is_graph
t.is_cluster=leda_graph.is_cluster

--Kernel functions shortcuts
t.getmetatable=kernel.getmetatable
t.setmetatable=kernel.setmetatable


t.couple=leda_connector.coupled
--t.cohort=leda_connector.cohort

--Exporting stage related functions
t.stage=leda_stage.new_stage
t.is_stage=leda_stage.is_stage



--Process start function
t.start=process.start

--Exporting cluster related functions
t.cluster=leda_cluster.new_cluster

return t
