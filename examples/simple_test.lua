require "leda"

local utils=require "leda.utils"

local graph,connector,stage=leda.graph,leda.connector,leda.stage

printer=stage{utils.print}

for i=1,100 do
   printer:send(i)
end

local g=graph{printer}

g:run()
