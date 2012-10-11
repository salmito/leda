-- sieve_stages.lua
-- Count the number of prime numbers between 1 and a limit N using
-- the sieve of Eratosthenes with blocks serially (block by block)
-- typical usage: lua -e "N=500 BLOCK=100" sieve_block.lua"

local function erasthosthenes_block(from, to)
	local size=(to - from + 1)
	local isprime={}
	for i=1,size,2 do
		isprime[i]=i+from-1
	end
	local i=3
   while i*i<=to do
		--skip some multiples
		if (i>=3*3 and i%3 ==0) or
		   (i>=5*5 and i%5 ==0) or
		   (i>=7*7 and i%7 ==0) or
		   (i>=11*11 and i%11 ==0) or
		   (i>=13*13 and i%13 ==0)
		then else
			--skip numbers before current block
			local minJ=(math.floor((from+i-1)/i))*i
			if minJ < i*i then minJ=i*i end
			--start value must be odd
         if minJ%2==0 then minJ=minJ+i end
			--find all non-primes multiples of i
         for j=minJ,to,2*i do
				if j>to then break end
				isprime[j-from+1]=nil
         end
		end
      i=i+2
	end
	if from <= 2 then isprime[1]=nil isprime[2]=2 end
	local n=0
   for p,v in pairs(isprime) do
   	n=n+1
		print(v)
	end
	return n
end


N=N or 500		-- from command line
BLOCK = BLOCK or 50000

local n=0

for from=1,N,BLOCK do
	local to=from+BLOCK-1
   if to > N then to=N end
   n=n+erasthosthenes_block(from, to)
end

--print(n)
