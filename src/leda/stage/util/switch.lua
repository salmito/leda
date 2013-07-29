local _=require 'leda'

-----------------------------------------------------------------------------
-- param:  output_key
--			  '...': data to be sent
-----------------------------------------------------------------------------

local stage={}

function stage.handler(output_key,...)
   local out=leda.get_output(output_key)
   if out then 
      out:send(...) 
   else
      error(string.format("Output '%s' does not exist",tostring(output_key)))
   end
end

stage.name="Switch"

return _.stage(stage)
