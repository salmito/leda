local _=require'leda'

if not pcall(require,'lfs') then
	error("'luafilesystem' is not installed")
end

return _.stage'Files'{
	init=function()
		if not pcall(require,'lfs') then
			error("'luafilesystem' is not installed")
		end
		require'string'
	end,
	handler=function(dir,str,...)
	   local lfs=require 'lfs'
	   local string=require'string'
	  	str=str or ".*"
      local n=0
      for file in lfs.dir(tostring(dir)) do
          local relative=dir..'/'..file
          if lfs.attributes(relative,"mode") == "file" then
          	if string.match(file,str) then
                print('file',file)
  	            if not leda.send(1,file,n+1,dir) then
                  leda.send('total',n)
	                return
	              end
                n=n+1
          	end
          end
      end
      leda.send('total',n,...)
	end,
}
