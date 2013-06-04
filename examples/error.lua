local utils=require "leda.utils"

local stim=leda.stage(function() while true do leda.send(1) leda.sleep(1) end end)
local err=leda.stage(function() error('some_error') end)

local g=leda.graph{leda.connect(stim,err)}

stim:send(i)

g:run()
