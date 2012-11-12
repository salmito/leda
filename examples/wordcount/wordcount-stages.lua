require 'leda'

local args={...}
local word=args[1] or error("Invalid parameter '#1' string expected")
local dir=args[2] or './dir'
local pattern=args[3] or '%w+'
local bl=args[4] or "false"
local threads=args[5] or 1
local maxpar=args[6] or 1

local line_reader=leda.stage{
	handler=function(file) 
--		local str_buf={}
      local f = io.open(file,"r")
		local buf,err=f:aread(4096)
		while buf do
--[[			local l=string.find(buf,'\n')
			if l then
				print("Achou \\n",l,string.sub(buf,1,l),string.sub(buf,l+1))
				table.insert(str_buf,string.sub(buf,1,l))
				leda.send("line",table.concat(str_buf))
				str_buf={string.sub(buf,l+1)}
			else
				table.insert(str_buf,buf)
			end
--]]
			leda.send("line",buf)
			buf,err=f:aread(4096)
		end
		f:close()
--   	leda.sleep(0.5)
		leda.send("line",nil)
	end,
	init=function()
		if bl=="true" then
			block=true
		end
		require "leda.utils.io"
		require "string"
		require "table"
	end
}

local pattern_count=leda.stage(function (line)
   local count=0
   -- repeat for each line in the file
   --print("line",line)
   if line==nil then
   	leda.send(1,nil)
   	return
   end
   for w in string.gfind(line, pattern) do
      -- call the function
      if w:find(word) then
         count = count + 1
      end
   end
   assert(leda.send(1,count))--return count
end,
function ()
   require "string"
end)


local counter = leda.stage{handler=function(local_counter)
   if not n then
      local lfs=require 'lfs'
      n=0 --number of expected events
      global_counter=0
      init_time=leda.gettime()
      for file in lfs.dir(tostring(dir)) do
          if lfs.attributes(dir..'/'..file,"mode") == "file" then 
            leda.send(1,dir..'/'..file,word)
            n=n+1
          end
      end
   elseif local_counter then
      global_counter = global_counter + local_counter
   elseif local_counter==nil then
   	n=n-1
   end
   if n==0 then
      print("Result",global_counter,leda.gettime()-init_time,bl,threads,maxpar)
      global_counter=0
      leda.quit()
      n=nil
   end
end, 
serial=true}

local g=leda.graph{
	start=counter,
	counter:connect(line_reader),
	line_reader:connect("line",pattern_count),
	line_reader:connect("end",counter),
	pattern_count:connect(counter)
}
pattern_count.name="Word count"
counter.name="Global Counter"
--g:plot('graph.png')
g:send()
g:run{controller=leda.controller.interactive.get(threads),maxpar=maxpar}
