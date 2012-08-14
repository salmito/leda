require "leda"
require "leda.utils.plot"

local fib=leda.stage{
	"Fibbonacci",
	handler=function (fib,n,val,oldval)
	   if not fib then error("Invalig argument for stage") end
	   
		val=val or 1
		oldval=oldval or 1
		n=n or 2

		if fib == 1 or fib == 2 then
			leda.send("value",fib,1)
		elseif n==fib then
			leda.send("value",fib,val)
		elseif n<fib then
			leda.send("loopback",fib,n+1,val+oldval,val)
		end
	end,
	bind=function(fib)
		assert(#fib.output.value.consumers>=1,"Value output must be connected to someone")
		fib:connect("loopback",fib)
		fib:method("loopback",leda.e)
	end,
}

local printer=leda.stage{
   leda.utils.print,
   name="Printer"
}

fib:connect("value",printer)

fib.output.value:method(leda.t)

local graph=leda.graph{fib,printer,proxy}

fib:send(tonumber(arg[1]))

leda.plot_graph(graph,"fib.png")

graph:run()
