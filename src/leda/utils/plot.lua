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
   local g = strictdigraph{
      tostring(leda_graph),
      compound = "1",
      textwrap="auto",    
      rankdir = "LR",
   }
   g.label=tostring(leda_graph)
   local start_node=nil
   local nodes={}
   if leda_graph.start then
      start_node=g:node{"START",shape='Mdiamond'}
   end
   local main_cluster=g:cluster{'Main'}
   main_cluster.label="Main"
   local clusters={}
   for s in pairs(leda_graph:stages()) do
      local sname=tostring(s)
      if s.serial then
         sname="["..sname.."]"
      end
      local cl=main_cluster
      local s_cl=leda_graph:get_cluster(s)
         clusters[s]=clusters[s] or g:cluster{tostring(s_cl)}
         cl=clusters[s]
         clusters[s].label=tostring(s_cl)
         if s_cl:is_serial() then
            clusters[s].label="["..tostring(s_cl).."]"
         end
      nodes[s]=cl:node{sname}
--      s.cluster=g:cluster("Sub"..tostring(leda_graph.stages[i]))
   end
   for c in pairs(leda_graph.conns) do
      local node=start_node
      if c.producer then
         node=nodes[c.producer]
      end
      local color=nil
      local style=nil
      local arrowType=nil
      if c:get_type()=='call' then
         color="#FF0000"
         arrowType="dot"
      elseif c:get_type()=='fork' then
         color="#0000FF"
         arrowType="invdot"
      elseif c:get_type()=='emmit' then
         if not c.producer or leda_graph:get_cluster(c.producer) ~= leda_graph:get_cluster(c.consumer) then
            style="dashed"
         end
      end
      g:edge{node,nodes[c.consumer],label=tostring(c),color=color,fontcolor=color,style=style,arrowhead=arrowType}
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
