local utils=require "leda.utils"

printer=stage{utils.print}

for i=1,10 do
   printer:send(i)
end

local g=graph{printer}

g:run()
