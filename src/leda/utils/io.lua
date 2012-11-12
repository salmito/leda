require "io"

local block=block or false

local file_mt,err=leda.getmetatable("FILE*")
if file_mt then
   file_mt.__wrap=function(file)
      local filefd=leda.io.wrap(file)
      return function()
         return leda.io.unwrap(filefd)
      end
   end
   if not block then
   	if type(leda.aio.do_file_aio)=='function' then
		  	file_mt.__index.aread=function(file,size)
		  		assert(leda.getmetatable(file)==file_mt);
		  		return leda.aio.do_file_aio(file,1,size)
		  	end
		  	file_mt.__index.awrite=function(file,buf)
		  		assert(leda.getmetatable(file)==file_mt);
		  		return leda.aio.do_file_aio(file,2,buf)
		  	end
		else
		  	file_mt.__index.aread=function(...) return nil,"AIO not available" end
   	end
   else
   	file_mt.__index.aread=file_mt.__index.read
   	file_mt.__index.awrite=file_mt.__index.write
   end
end
