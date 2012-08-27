local utils=require "leda.utils"

local a={test="testing"}

local s1=stage{
   handler=function()
	   local b="xxx"
           leda.send(1,a,function () return b end,io.stderr)
   end,
   init = function () require "io" end
}

local s2=stage{
   handler=function(test,f)
      print(test.test,f())
   end
}

s1:connect(s2)

s1:send()

graph{s1,s2}:run()
