local _=require 'leda'

local stage={}

-----------------------------------------------------------------------------
-- Roundrobin through its output ports 
--
-- param:   '...': data to be sent
-----------------------------------------------------------------------------

function stage.handler(...)
      coroutine.resume(c,...)
end
  
function stage.init()
    local f=function()
      while true do
         for k,p in pairs(leda.output) do
            p:send(coroutine.yield())
         end
      end
   end
   c=coroutine.create(f)
   coroutine.resume(c)
end

stage.stateful=true
--stage.autostart=true

stage.name="RoundRobin"

return _.stage(stage)
