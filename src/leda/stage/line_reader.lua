require 'leda'

local stage={}

function stage.handler(file,...)
   local f = assert(io.open(file,"r"))
	local buf,err=f:aread(4096)
	local i=1
	local t={}
	while buf do
      for k,v in string.gmatch(buf,"([^\n]*)([\n]?)") do
         if v=='\n' then
            table.insert(t,k)
            leda.send("line",table.concat(t),i,file,...)
            i=i+1
            t={}
      	   elseif k~='' then
         	   table.insert(t,k)
         end
      end
      buf,err=f:aread(4096)
   end
   f:close()
   leda.send("EOF",'EOF',file,...)
end

function stage.init()
   require 'table'
   require 'string'
   require 'leda.utils.io'
end

function stage.bind(self) 
   assert(self.line,"File output must be connected")
end

stage.serial=false --Not serial

stage.name="Line feeder"

stage.description=[[
Receives a filename as input and outputs 
each line of file to the 'line' port along with a line counter and the filename.
EOF event is emmited at the end of file.

Uses asynchronous IO
         
Requires 'line' port to be connected
]]

stage.version='0.1'

return leda.stage(stage)
