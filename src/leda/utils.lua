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
-----------------------------------------------------------------------------
-- Insert a stage
-- param:   's1':     
--          's2':   
--
-- returns: 
-----------------------------------------------------------------------------
function utils.insert_before(s1,s2,key)
   key=key or 1
   s1.input=s2.input
   s2.input=connector{}
   s1:add_output(key,s2.input)
end

function utils.insert_after(s1,s2,key)
   key=key or 1
   s1.output[key]=connector()
   s2.input=s1.output[key]
end

function utils.add_before(s1,s2,key)
   s1.input=s2.input
   s1:add_output(key,s2.input)
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
-- Loop indefinitely sending the input event to its output
-- param:   'i':     period of loop
-- param:   '...':   data to be sent at
-----------------------------------------------------------------------------
utils.loop={
   handler= function (...) 
      local args={...} 
      while true do 
         leda.output[1]:send(select(2,...)) 
         leda.sleep(args[1]) 
      end 
   end,
   bind=function(self) 
      assert(self.output[1],"Copy must have an output at [1]") 
   end
}

-----------------------------------------------------------------------------
-- Print received data
-- param:   '...':   data to be printed
-----------------------------------------------------------------------------
utils.print={
   handler=
   function (...)
      print(...)
   end
}

-----------------------------------------------------------------------------
-- Serialize arguments into a string
-- param:   '...':   data to be serialized
-----------------------------------------------------------------------------
--[[utils.simple_serializer={
   handler=
   function (...)
      leda.get_output(1):send(serialize{...})
   end,
   init=
   function ()
      function serialize (tt, indent, done)
         done = done or {}
         indent = indent or 0
         if type(tt) == "table" then
            for key, value in pairs (tt) do
               io.write(string.rep (" ", indent)) -- indent it
               if type (value) == "table" and not done [value] then
                  done [value] = true
                  io.write(string.format("[%s] => table\n", tostring (key)));
                  io.write(string.rep (" ", indent+4)) -- indent it
                  io.write("(\n");
                  table_print (value, indent + 7, done)
                  io.write(string.rep (" ", indent+4)) -- indent it
                  io.write(")\n");
               else
                  io.write(string.format("[%s] => %s\n",
                  tostring (key), tostring(value)))
               end
            end
         else
            io.write(tt .. "\n")
         end
      end
   end,
   bind=
   function(self) 
      assert(#self.output>1,"Serializer must have an output at [1]") 
   end
}--]]

return utils
