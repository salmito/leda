local _=require 'leda'

return _.stage{
   name='Process term',
   handler=function (...)
       leda.quit(...)
   end,
   serial=true
}

