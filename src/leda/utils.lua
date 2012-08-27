require "leda"

-----------------------------------------------------------------------------
-- Defining globals for easy access to the leda main API functions
-----------------------------------------------------------------------------
stage,connector,graph=stage or leda.stage,connector or leda.connector,graph or leda.graph

leda.utils={}
local utils=leda.utils

-----------------------------------------------------------------------------
-- Defining auxiliary functions
-----------------------------------------------------------------------------
function utils.insert_proxy(s2,s1,key1,key2)
   key1=key1 or 1
   key2=key2 or 1
   local con=s1.output[key1] or leda.connector()
   s1.output[key1]=leda.connector()
   s2.input=s1.output[key1]
   s2.output[key2]=con
end

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
         error(string.format("Output '%s' does not exist",tostring(n)))
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
      end end
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
-- Print received data
-- param:   '...':   data to be printed
-----------------------------------------------------------------------------
utils.print={
	"Printer",
   handler=
   function (...)
      print(...)
      leda.send(1,...)
   end
}

-----------------------------------------------------------------------------
-- Serialize arguments into a string
-- param:   '...':   data to be serialized
-----------------------------------------------------------------------------
utils.serializer={
   handler=
   function (...)
      local str=leda.encode({...})
      leda.send(1,str)
   end,
   bind=
   function(self) 
      assert(#self.output[1].consumers>0,"Serializer must have a consumer at output [1]") 
   end
}

utils.make_pipeline=function(...)
   local arg={...}
   local s={"Linear Pipeline"}
   local last_s=nil
   for i,v in ipairs(arg) do
      assert(type(v)=="function","Argument #"..tostring(i).." must be a function")
      local stage=leda.stage{"Stage "..tostring(i),handler=v}
      s:insert(stage)
      if last_s then last_s:connect(stage) end
      last_s=stage
   end
   return leda.graph(s)
end

return utils
