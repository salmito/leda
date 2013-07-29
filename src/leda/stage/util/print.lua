local _=require 'leda'

return _.stage{
   handler=function (...)
               print(...)
               leda.send(1,...)
           end,
   init=function()
   	print(self.name)
   end,
   name="Console print",
}
