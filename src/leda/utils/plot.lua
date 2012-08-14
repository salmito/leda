local utils=require "leda.utils"

local tostring = tostring
local pcall = pcall
local antgr=graph
graph=nil
module ("leda.utils.plot", package.seeall)
local gr = require "graph"
graph=antgr

local node, edge, subgraph, cluster, digraph, strictdigraph =
  gr.node, gr.edge, gr.subgraph, gr.cluster, gr.digraph, gr.strictdigraph

function plot_graph(leda_graph,out)
   if not leda_graph:verify() then
      return nil, "Error verifying graph"
   end
   local g = strictdigraph{
      tostring(leda_graph),
      compound = "1",
      rankdir = "LR"
   }
   for i=1,#leda_graph.stages do
      local sname=tostring(leda_graph.stages[i])
      if leda_graph.stages[i].serial then
         sname=sname.."*"
      end
      leda_graph.stages[i].node=g:node(sname)
      leda_graph.stages[i].cluster=g:cluster("Sub"..tostring(leda_graph.stages[i]))
   end
   for i=1,#leda_graph.connectors do
      if leda_graph.connectors[i].type=="e" or leda_graph.connectors[i].type=="u" then
         for p=1,#leda_graph.connectors[i].producers do
            local head=leda_graph.connectors[i].producers[p].node
            for c=1,#leda_graph.connectors[i].consumers do
               local tail=leda_graph.connectors[i].consumers[c].node
               g:edge{head,tail,label=tostring(leda_graph.connectors[i])}
            end
         end
      elseif leda_graph.connectors[i].type=="t" or leda_graph.connectors[i].type=="te" then
         for p=1,#leda_graph.connectors[i].producers do
            local head=leda_graph.connectors[i].producers[p].node
            for c=1,#leda_graph.connectors[i].consumers do
               leda_graph.connectors[i].producers[p].cluster:edge{head,leda_graph.connectors[i].consumers[c].node,label=tostring(leda_graph.connectors[i])}
            end
         end
      end
      
   end
   if not out then
      g:show()
   elseif type(out)=="string" then
      ext=out:reverse():gmatch("[^\.]*")():reverse()
      if ext then
         g:layout()
         g:render(ext, out)
      end
   else
      error("Invalid parameter type")
   end
   g:close()
end
leda.plot_graph=plot_graph
