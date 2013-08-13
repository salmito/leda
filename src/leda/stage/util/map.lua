local _=require 'leda'

local stage={}

function stage.handler(vec,...)
   for k,v in ipairs(vec) do
      vec[k]=self.f(v)
   end
   leda.send(vec,...)
end

function stage.init()
   leda.loadlibs()
end

function stage.bind(self,out,graph) 
   assert(out[1],"Output port must be defined")
   assert(type(self.f)=='function',"Field f must be a function")
end

stage.name="Map"

return _.stage(stage)
