local memarray=require "leda.lmemarray"

local mt,err=leda.getmetatable('leda_memarray*')

if mt then
   mt.__wrap=function(array)
      local type_t=array:type()
      local length=array:length()
      local ptr=array:to_ptr()
      --print('wraping')
      return function()
	      local memarray=require 'leda.memarray'
	      local f=memarray.from_ptr(ptr,type_t,length)
         return f
      end
   end
   mt.__persist=function(array)
      local str=array:to_str()
      local type_t=array:type()
      local length=array:length()
      --print('persisting')
      return function()
         local memarray=require 'leda.memarray'
         return memarray(type_t,length):from_str(str)
      end
   end
end

return memarray
