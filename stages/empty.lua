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

stage._DESCRIPTION=[[
]]

stage._VERSION='0.1'

return leda.stage(stage)
