local M=require("imlib2")
local meta=leda.getmetatable('imlib2.image')
if meta then
   meta.__wrap = function (img)
      local ptr=img:to_ptr()
      return function ()
         return imlib2.image.from_ptr(ptr)
      end
   end
end
return M
