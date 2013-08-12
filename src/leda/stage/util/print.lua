local _=require 'leda'

return _.stage{
   handler=function (...)
               print(...)
               leda.send(1,...)
           end,
   init=function()
   end,
   name="Console print",
}
