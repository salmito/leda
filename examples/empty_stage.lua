require 'leda'

local stage={}

function stage.handler(file,...)
   print('stage')
end

function stage.init()
end

function stage.bind(self) 
end

stage.serial=false

stage.name="Stage"

stage.description=[[
]]

stage.version='0.1'

return leda.stage(stage)
