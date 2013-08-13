local _=require 'leda'

return _.stage{
   handler=function (...)
               print(...)
               leda.push(...)
           end,
   serial=true,
   name="Console print",
}
