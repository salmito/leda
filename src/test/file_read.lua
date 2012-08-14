require "leda.utils"

s1=stage{
   handler=function(file)
      local a=io.open(file,"r")
      local line = a:read()
      while line do
         leda.get_output():send(line)
         line = a:read()
      end
   end,
	init=function() require "io" end
}

s2=stage{leda.utils.print}

leda.connect(s1,s2)

g=graph{s1,s2}

s1:send("Makefile")

g:run()
