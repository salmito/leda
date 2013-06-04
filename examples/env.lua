local utils=require "leda.utils"

local n=10

printer=leda.stage(function(i) 
	j=(j or 0)+1 --global j 
	print(i,j) 
	if i==n then leda.quit() end 
end)

local g=leda.graph{start=printer}

for i=1,n do
   printer:send(i)
end

g:run{maxpar=2}
