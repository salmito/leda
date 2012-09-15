require 'socket'
local args={...}
local word=args[1] or error("Invalid parameter '#1' string expected")
local dir=args[2] or './dir'
local pattern=args[3] or '%w+'

local function word_count (file,word)
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
   return count
end

require'lfs'

local global_count=0
local init_time=socket.gettime()
for file in lfs.dir(tostring(dir)) do
    if lfs.attributes(dir..'/'..file,"mode") == "file" then 
          global_count = global_count + word_count(dir..'/'..file,word)
    end
end

print(global_count,socket.gettime()-init_time)

