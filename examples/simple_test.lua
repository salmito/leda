local utils=require "leda.utils"

printer=leda.stage(function(i) print(i) if i==10 then leda.quit() end end)

local g=leda.graph{start=printer}

for i=1,10 do
   printer:send(i)
end

g:run()
