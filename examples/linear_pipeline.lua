local leda=require "leda"
local utils=require "leda.utils"

local a=leda.stage(function(i,...) if type(i)=='number' then print(i,...) leda.push(i+1,...) end end)

local finish=leda.stage"Finish"(function() leda.quit() end)

local g=utils.linear_pipeline(a,a,a,a,a,a,a,a,a,a,finish)

g.start:push(1,'Flux')

g:run()
