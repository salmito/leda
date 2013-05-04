require 'leda'

local stage={}

function stage.handler(...)
   print('stage')
end

function stage.init()
end

function stage.bind(self,out,graph) 
end

stage.serial=false

stage.name="Stage"

return leda.stage(stage)
