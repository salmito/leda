local leda=require "leda"

local stim=leda.stage(function() i=0 while i<10 do leda.send(1,i) leda.sleep(1) i=i+1 end leda.sleep(1) leda.quit() end)
local err=leda.stage"Fail Stage"(function(i) error('some_error'..i) end)

local g=leda.graph{leda.connect(stim,err)}

stim:send(i)

g:run()
