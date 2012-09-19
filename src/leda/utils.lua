require "leda"

-----------------------------------------------------------------------------
-- Defining globals for easy access to the leda main API functions
-----------------------------------------------------------------------------
leda.utils={}
local utils=leda.utils

-----------------------------------------------------------------------------
-- Switch stage selects an specific output and send pushed 'data'
-- param:   'n':     output key to push other arguments
--          '...':   data to be sent
-----------------------------------------------------------------------------
utils.switch={
   handler=function (...)
      local args={...}
      local out=leda.get_output(args[1])
      if out then 
         out:send(select(2,...)) 
       else
         error(string.format("Output '%s' does not exist",tostring(args[1])))
      end
   end
}

-----------------------------------------------------------------------------
-- Broadcast stage select send pushed 'data' to all of its outputs
-- param:   '...': data to be broadcasted
-----------------------------------------------------------------------------
utils.broadcast={
   handler=function (...)
   for _,connector in pairs(leda.output) do
           connector:send(...)
      end 
   end
}

-----------------------------------------------------------------------------
-- Copy stage sends N copies of each event it receives to its output
-- param:   'n':     number of copies to push
-- param:   '...':   data to be copyied and sent
-----------------------------------------------------------------------------
utils.copy={
   handler=function (...)
      local args={...} for i=1,args[1] do
         leda.output[1]:send(select(2,...))
      end end,
   bind=function(self) 
      assert(self.output[1],"Copy must have an output at [1]") 
   end
}

-----------------------------------------------------------------------------
-- Load  and execute a lua chunk
-- param:   'str'    chunk to be loaded
-- param:   '...':   data to be copyied and sent
-----------------------------------------------------------------------------
utils.eval={
   handler=function (...)
      local args={...}
      loadstring(args[1])(select(2,...))
   end,
}

-----------------------------------------------------------------------------
-- Print received data and pass it if connected
-- param:   '...':   data to be printed
-----------------------------------------------------------------------------
utils.print={
   handler=function (...)
      print(...)
      leda.send(1,...)
   end
}

utils.linear_pipeline=function(...)
   local arg={...}
   local g=leda.graph{}

   local last_s=nil

   for i,v in ipairs(arg) do
      assert(type(v)=="function" or leda.is_stage(v),"Argument #"..tostring(i).." must be a function or a stage")
      local stage=leda.stage(v)
      if last_s then g:add(last_s:connect(stage)) 
      else g.start=stage end
      last_s=stage
   end
   return g
end

return utils
