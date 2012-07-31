local utils=require "leda.utils"

local s1=stage{
   handler=function()
      leda.send(1,leda.marshal.encode({test="testing"}))
   end
}

local s2=stage{
   handler=function(table)
      local test=leda.marshal.decode(table)
      print(test.test)
   end
}

utils.insert_before(s1,s2)

s1:send()

graph{s1,s2}:run()
