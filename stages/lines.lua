require 'leda'

local stage={}

function stage.handler(file,...)
   local f,err = io.open(file,"r")
   if f==nil then
      leda.send("error",err)
      error(err)
   end
	local buf,err=f:aread(4096)
	local i=0
	local t={}
	while buf do
      for k,v in string.gmatch(buf,"([^\n]*)([\n]?)") do
         i=i+1
         if v=='\n' then
            table.insert(t,k)
            leda.send("line",table.concat(t),i,file,...)            
            t={}
      	   elseif k~='' then
         	   table.insert(t,k)
         end
      end
      buf,err=f:aread(4096)
   end
   f:close()
   --Send EOF signal including the number of lines read and the filename
   leda.send("EOF",'EOF',i-1,file,...)
end

function stage.init()
   table=require 'table'
   string=require 'string'
   io=require 'leda.utils.io'
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

stage.test=function(file)
   local utils=require 'leda.utils'
   local s=leda.stage(stage)
   s:send(file or '/etc/passwd')
   local g=leda.graph{
      s:connect('line',leda.stage'printer'("print(...)")),
      s:connect('EOF',leda.stage'quitter'('leda.quit()'))
   }
   --g:plot()
   g:run()
end

return leda.stage(stage)
