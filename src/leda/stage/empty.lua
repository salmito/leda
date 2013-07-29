local _=require 'leda'

local stage={}

function stage.handler(...)
end

function stage.init()
end

function stage.bind(self,out,graph) 
end

stage.serial=false

stage.autostart=false

stage.name="Empty stage"

return _.stage(stage)
