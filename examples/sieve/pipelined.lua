-- sieve_stages.lua
-- Count the number of prime numbers between 1 and a limit N using
-- the sieve of Eratosthenes with blocks programmed with stages 
-- typical usage: lua -e "N=500 BLOCK=100" sieve_stages.lua"

require "leda"
require "leda.controller.fixed_thread_pool"

local N=N or 500		-- from command line

function get_stage(n)
	return {
		handler=function(val)
			if val%n~=0 then
				--print(val,n)
				leda.send("val",val)
			end
		end,
		init=function()
			require "math"
		end,
--		stateful=true,
		name="Multiple filter "..n
	}
end

local connectors={}

local last_s=nil
local first_s=nil

function erato(n)
  local t = {0, 2}
  for i = 3, n, 2 do t[i], t[i+1] = i, 0 end
  for i = 3, math.sqrt(n) do for j = i*i, n, 2*i do t[j] = 0 end end
  return t
end

--for k,v in pairs(erato(math.sqrt(N))) do print(k,v) end

local t = erato(math.sqrt(N))		-- generate primes up to N
print(2)
for i=3,#t,2 do
	if t[i]~=0 then
	  print(i)		-- must be a prime number
     local s=leda.stage(get_stage(i))
   	if last_s then
		   table.insert(connectors,last_s:connect("val",s))
   	else
	   	first_s=s
		end
		last_s=s
	end
end


local printer=leda.stage{handler=
	function(val)
		n = n or 0
		n=n+1
--		print(val)
		print(val,n)
--		if n==1204 then
--			leda.quit()
--		end
	end,stateful=true,init=function() require "table" end}

local init_stage=leda.stage{
	handler=function() 
		local init=math.floor(math.sqrt(N))
		if init%2==0 then --init must be even
			init=init+1
		end
		for i=init,N,2 do
			leda.send('val',i)
		end
	end,
	init=function() require "math" end
}

table.insert(connectors,last_s:connect("val",printer))
table.insert(connectors,init_stage:connect("val",first_s))

local g=leda.graph(connectors)

init_stage:send()

io.stderr:write("Running\n")
g:run{controller=leda.controller.interactive.get(4),maxpar=4}
--g:run{controller=leda.controller.fixed_thread_pool.get(8),maxpar=1} --]]
