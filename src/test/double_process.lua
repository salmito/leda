local exec=[[lua -l leda -e "leda.start{port=8888}"&]]
os.execute(exec)
require 'leda'
leda.kernel.sleep(1)

local n=n or 1000

local s1=leda.stage{
	handler=function()
		local t={}
		for i=1,n do
			t[#t+1]=i
		end
		 while true do
   		leda.send(1,t)
   		--leda.nice()
       end
	end
}

local s2=leda.stage{
	handler=function(t)
		print('received',t)
	end
}

local it=it or 1

for i=1,it do
	 s1:send(1)
end

local g=leda.graph{s1:connect(1,s2)}

g:part(s1,s2)
g:map('localhost','localhost:8888')

g:run()
