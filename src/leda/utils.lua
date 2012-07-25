utils={}

-----------------------------------------------------------------------------
-- Switch stage selects an specific output and send pushed 'data'
-- param:   'n':     output key to push other arguments
--          '...':   data to be sent
-----------------------------------------------------------------------------
utils.switch={
   handler=
   [[--function (n,...)
      local out=leda.get_output(n)
      if out then 
         out:send(...) 
      else
         error(string.format("Output '%s' does not exist",tostring(n)))
      end
   --end]]
}

-----------------------------------------------------------------------------
-- Broadcast stage select send pushed 'data' to all of its outputs
-- param:   '...': data to be broadcasted
-----------------------------------------------------------------------------
utils.broadcast={
   handler=
   [[--function (...)
      local out=leda.get_output()
      for _,connector in pairs(out) do
           connector:send(...)
      end
   --end]]
}

-----------------------------------------------------------------------------
-- Copy stage sends N copies of each event it receives
-- param:   'n':     number of copies to push
-- param:   '...':   data to be copyied and sent
-----------------------------------------------------------------------------
utils.copy={
   handler=
   [[--function (n,...)
      for i=1,n do
         leda.get_output(1):send(...)
      end
   --end]],
   bind=function(self) 
      assert(#self.output>1,"Copy must have an output at [1]") 
   end
}

-----------------------------------------------------------------------------
-- Print received data
-- param:   '...':   data to be printed
-----------------------------------------------------------------------------
utils.print={
   handler=
   [[--function (...)
      print(...)
   --end]]
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
