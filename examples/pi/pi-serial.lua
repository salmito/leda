require "socket"
require "fast_rand"



local iit=1024

seed(os.time(),os.time())

local n,it =tonumber(arg[1]),tonumber(arg[2])


while iit<it do
init=socket.gettime()
local count=0
for i=1,iit do
   local x,y=rand(),rand()
   local z=x*x+y*y
   if z<=1 then count=count+1 end
end

local pi=(count*4)/iit

local cpu_time=socket.gettime()-init

io.stderr:write(string.format("pi_serial\t%f\t%.12f\t%f\t%d\t%d\n",pi,math.abs(math.pi-pi),cpu_time,iit,1))   

iit=iit*2
end

