-----------------------------------------------------------------------------
-- Leda main module
-----------------------------------------------------------------------------
--[[ @name leda
module "leda"
]]-----------------------------------------------------------------------------

-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura

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

------------------------------------------------------------------------
-- Bind two stages through a connector in a given output port
-- @class function
-- @name leda.connect
-- @param from_stage stage: The stage on the head of the connector
-- @param port number, string or nil, The output port of the head stage, 
-- if omitted, output port will be set to 1
-- @param to_stage stage: The destination stage of the connector
-- @return A connector construction function that will be called by
-- the graph.
------------------------------------------------------------------------
t.connect=leda_stage.connect
t.process=process
t.controller={}

local print,pairs=print,pairs

------------------------------------------------------------------------
-- Get the current system time
-- @class function
-- @name leda.gettime
-- @return Current time (unix timestamp with microsecond resolution).
------------------------------------------------------------------------
t.gettime=kernel.gettime

--Exporting graph related functions
------------------------------------------------------------------------
-- Create a new Leda graph
-- @class function
-- @name leda.graph
-- @param ... A list of connectors to form the graph
-- @return graph: A table associated with a graph object.
------------------------------------------------------------------------
t.graph=leda_graph.graph
------------------------------------------------------------------------
-- Test if the parameter is a graph object
-- @class function
-- @name leda.is_graph
-- @param g A lua variable to be tested
-- @return true if the passed object is a graph, false if not
------------------------------------------------------------------------
t.is_graph=leda_graph.is_graph
------------------------------------------------------------------------
-- Test if the parameter is a cluster object
-- @class function
-- @name leda.is_cluster
-- @param cl A lua variable to be tested
-- @return true if the passed object is a cluster, false if not
------------------------------------------------------------------------
t.is_cluster=leda_graph.is_cluster

--Kernel functions shortcuts
t.getmetatable=kernel.getmetatable
t.setmetatable=kernel.setmetatable


t.couple=leda_connector.coupled
--t.cohort=leda_connector.cohort

--Exporting stage related functions
------------------------------------------------------------------------
-- Create a new stage
-- @class function
-- @name leda.stage
-- @param handler function or table: A function to be used as the event handler of the stage.
-- if this value is a table, it will use it as the source of all defined parameters.
-- @param init function: A function to be used to initiate the state of instances
-- @param bind function: A function to validate defined output port of a stage
-- @param name string: An abstract name of the stage
-- @param serial boolean: If true the stage will have persistent state and wont be parallel
-- @param autostart any type: A value to be passed to the event handler upon graph
-- execution. A nil or false value disable autostart.
-- @return A Leda stage object.
-- @usage local s1=leda.stage{<br />
-- &nbsp;&nbsp;&nbsp;handler=function() end, <br />
-- &nbsp;&nbsp;&nbsp;init=function() end, <br />
-- &nbsp;&nbsp;&nbsp;name='Stage1', <br />
-- &nbsp;&nbsp;&nbsp;serial=false, <br />
-- &nbsp;&nbsp;&nbsp;autostart=true <br /> }
------------------------------------------------------------------------
t.stage=leda_stage.new_stage

------------------------------------------------------------------------
-- Test if the parameter is a stage object
-- @class function
-- @name leda.is_stage
-- @param s A lua variable to be tested
-- @return true if the passed object is a stage, false if not
------------------------------------------------------------------------
t.is_stage=leda_stage.is_stage

------------------------------------------------------------------------
-- Start a new process daemon.
-- @class function
-- @name leda.start
-- @param port integer: The TCP port of the current process.
-- if this value is a table, it will use it as the source of all defined parameters.
-- @param host string: Define the localhost of the daemon, default is 'localhost'.
-- @param controller controller: A controller definition to be used by this process.
-- @param maxpar integer: A default value for the maximum number of parallel instace of all stages
-- @return This function never returns
-- @usage leda.start{controller=c1,maxpar=32,port=9999,host='myhostname.domain.com'}
------------------------------------------------------------------------
t.start=process.start

--Exporting cluster related functions
------------------------------------------------------------------------
-- Create a new cluster of stages.
-- @class function
-- @name leda.cluster
-- @param ... A list of stages that form the cluster
-- @return A set of stages defining the cluster
-- @usage local c1=leda.cluster(s1,s2,s3) or leda.cluster{s1,s2,s3}
------------------------------------------------------------------------
t.cluster=leda_cluster.new_cluster

return t
