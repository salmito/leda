local _=require 'leda'

-----------------------------------------------------------------------------
-- Broadcast stage select send pushed 'data' to all of its outputs
-- param:   '...': data to be broadcasted
-----------------------------------------------------------------------------

local stage={}

function stage.handler(...)
   for _,connector in pairs(leda.output) do
         connector:send(...)
   end 
end

stage.name="Broadcast"

return _.stage(stage)
