local utils=require "leda.utils"

local a={test="testing"}

local s1=stage{
   handler=function()
		leda.send(1,a)
   end
}

local s2=stage{
   handler=function(test)
      print(test.test)
   end
}

s1:connect(s2)

s1:send()

graph{s1,s2}:run()
