require "io"

local file_mt,err=leda.getmetatable("FILE*")
if file_mt then
   file_mt.__wrap=function(file)
      local filefd=leda.io.wrap(file)
      return function()
         return leda.io.unwrap(filefd)
      end
   end
end
