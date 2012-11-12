-- sieve_stages.lua
-- Count the number of prime numbers between 1 and a limit N using
-- the sieve of Eratosthenes with blocks programmed with stages 
-- typical usage: lua -e "N=500 BLOCK=100" sieve_stages.lua"

require "leda"
require "leda.controller.fixed_thread_pool"

local threads=threads or 4
local maxN=maxN or 500
local max_primes_per_stage=max_primes or 10
local testcase=testcase or "Default"
local ctype=ctype or 'local'
local init_time=leda.gettime()

local counter=leda.stage{
	handler=function(prime)
--		print(prime)
		local n=inc()
		p=p or 0
		if 100*n/maxN > p or p==0  then
			print(testcase,"partial",n,maxN,100*n/maxN, max_primes_per_stage, leda.gettime()-init_time,threads,ctype)
			p=p+10;
		end
		if n==maxN then 
			print(testcase,"total_time",n, maxN,100, max_primes_per_stage,leda.gettime()-init_time,threads,ctype)
			leda.quit()
		end
	end,
	init=function ()
		local c=0
		max=2
		print(testcase,"bootstrap",0,maxN,max_primes_per_stage,leda.gettime()-init_time,threads,ctype)
		function inc() c=c+1 return c end
	end,
	stateful=true,
	name="Prime Counter"
}

local last_stage=nil
local first_stage=nil

local h=function (val)
	primes = primes or {}

	for _,p in ipairs(primes) do
		if val%p == 0 then --Divisible
--			print("Divisible",val,p)
			return
		end
	end

	if #primes<max_primes_per_stage then --we have a new prime
		table.insert(primes,val)
--		print("Got prime",val)
		leda.send('prime',val)
		return
	end
	leda.send('val',val)
end

local conn={}

io.stderr:write("Creating stages\n")
for i=1,maxN/max_primes_per_stage do
	local s=leda.stage{handler=h,init=function () require 'table' end,stateful=true,name=i}
	if last_stage then
		table.insert(conn,last_stage:connect('val',s))
	else
		first_stage=s
	end
	table.insert(conn,s:connect('prime',counter,ctype))
	last_stage=s
end

io.stderr:write("Created "..maxN/max_primes_per_stage.." stages\n")

local init_stage=leda.stage{"Odd number generator",
	handler=function () 
		leda.send('prime',2)
		local i = 3
		while true do
--			if (i>=3*3 and i%3 ==0) or
--			   (i>=5*5 and i%5 ==0) or
--			   (i>=7*7 and i%7 ==0) or
--			   (i>=11*11 and i%11 ==0) or
--			   (i>=13*13 and i%13 ==0) then
--			else
				leda.send('val',i)
--				leda.nice()
--			end
			i=i+2
		end
	end
}

table.insert(conn,init_stage:connect("prime",counter))
table.insert(conn,init_stage:connect("val",first_stage,ctype))

io.stderr:write("Creating graph\n")
local g=leda.graph(conn)
g.name="Sieve "..maxN.."x"..max_primes_per_stage.." ("..maxN/max_primes_per_stage.." stages)"
io.stderr:write("Created graph\n")

--io.stderr:write("Ploting graph\n")
--g:plot("sieve_"..maxN.."x"..max_primes_per_stage..".svg")
--g:plot()
--io.stderr:write("Ploted graph\n")

init_stage:send()

io.stderr:write("Running\n")
--g:run{controller=leda.controller.interactive.get(4,false),maxpar=4}
g:run{controller=leda.controller.fixed_thread_pool.get(threads),maxpar=1} --]]
