local _=require 'leda'

local stage={}

function stage.handler(file,...)
	local bufsize=2^12
   local f,err = io.open(file,"r")
   if f==nil then
      leda.send("error",err)
      error(err)
   end
	local buf,err=f:aread(bufsize)
	local i=0
	local t={}
	while buf do
      for k,v in string.gmatch(buf,"([^\n]*)([\n]?)") do
         i=i+1
         if v=='\n' then
            table.insert(t,k)
            leda.push(table.concat(t),i,file,...)            
            t={}
         elseif k~='' then
         	   table.insert(t,k)
         end
      end
      buf,err=f:aread(bufsize)
   end
   f:close()
   --Send EOF signal including the number of lines read and the filename
   leda.send("EOF",'EOF',i>0 and i-1 or 0,file,...)
end

function stage.init()
   table=require 'table'
   string=require 'string'
   aync=true
   io=require 'io'
end

function stage.bind(self,out) 
   assert(out[1],"Default output must be connected")
end

stage.name="Line feeder"

return _.stage(stage)
