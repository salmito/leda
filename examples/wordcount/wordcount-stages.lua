require 'leda'

local args={...}
local word=args[1] or error("Invalid parameter '#1' string expected")
local dir=args[2] or './dir'
local pattern=args[3] or '%w+'

local pattern_count=leda.stage(function (file,word)
   local count=0
   -- repeat for each line in the file
   for l in io.lines(file) do
      -- repeat for each word in the line
      for w in string.gfind(l, pattern) do
         -- call the function
         if w:find(word) then
            count = count + 1
         end
      end
   end
   assert(leda.send(1,count))--return count
end,
function ()
   require "string"
   require "io"
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
      n=n-1
   end
   if n==0 then
      print("Result",global_counter,leda.gettime()-init_time)
      global_counter=0
      n=nil
      assert(leda.send('result',global_counter) or leda.quit())
   end
end, 
serial=true}

local g=leda.graph{start=counter,counter:connect(pattern_count),pattern_count:connect(counter)}
pattern_count.name="Word count"
counter.name="Global Counter"
g:plot('graph.png')
g:send()
g:run{controller=leda.controller.interactive.get(5),maxpar=5}
