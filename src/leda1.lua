local stage=require 'leda.stage'
local core=require 'leda.core'

local t={}

t.stage=function(...)
   local arg={...}
   if type(arg[1])=='table' and #arg==1 then
      arg=arg[1]
   end
   return stage.new(core.encode(arg))
end

return t
