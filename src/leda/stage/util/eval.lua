local _=require 'leda'

local stage={}

-----------------------------------------------------------------------------
-- Load  and execute a lua chunk dinamicaly
-- param:   'chunk'    chunk to be loaded
-- param:   '...':   parameters passed when calling chunk
-----------------------------------------------------------------------------

function stage.handler(chunk,...)
   assert(loadstring(chunk))(...)
end

stage.name="Eval"

return _.stage(stage)
