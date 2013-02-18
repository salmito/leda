require "leda"

-----------------------------------------------------------------------------
-- Defining globals for easy access to the leda main API functions
-----------------------------------------------------------------------------
leda.utils={}
local utils=leda.utils

-----------------------------------------------------------------------------
-- Switch stage selects an specific output and send pushed 'data'
-- param:   'n':     output key to push other arguments
--          '...':   data to be sent
-----------------------------------------------------------------------------
utils.switch={
   name="Switch",
   handler=function (output_key,...)
      local args={...}
      local out=leda.get_output(output_key)
      if out then 
         out:send(...) 
       else
         error(string.format("Output '%s' does not exist",tostring(output_key)))
      end
   end
}

-----------------------------------------------------------------------------
-- Broadcast stage select send pushed 'data' to all of its outputs
-- param:   '...': data to be broadcasted
-----------------------------------------------------------------------------
utils.broadcast={
   name="Broadcast",
   handler=function (...)
   for _,connector in pairs(leda.output) do
           connector:send(...)
      end 
   end
}

-----------------------------------------------------------------------------
-- 
-- param:   '...': data to be sent
-----------------------------------------------------------------------------
utils.roundrobbin={
   name="RoundRobbin",
   handler=function(...)
      coroutine.resume(c,...)
   end,
   init=function()
      local f=function()
         while true do
            for k in pairs(leda.output) do
               leda.send(k,coroutine.yield())
            end
         end
      end
      c=coroutine.create(f)
      coroutine.resume(c)
   end,
   stateful=true

}

-----------------------------------------------------------------------------
-- Load  and execute a lua chunk
-- param:   'str'    chunk to be loaded
-- param:   '...':   parameters passed when calling chunk
-----------------------------------------------------------------------------
utils.eval={
   name="Eval",
   handler=function (chunk,...)
      assert(loadstring(chunk))(...)
   end,
}

-----------------------------------------------------------------------------
-- Print received data and pass it if connected
-- param:   '...':   data to be printed
-----------------------------------------------------------------------------
utils.print={
   name="Print",
   handler=function (...)
      print(...)
      leda.send(1,...)
   end
}


utils.deflate={
   name='Lossless compressor',
   handler=function(str,...)
      local stream = zlib.deflate()
      local compressed_str,eof,bytes_in,bytes_out=stream(str,'full')
      leda.send('data',compressed_str,...)
   end,
   init="require 'zlib'"
}

utils.inflate={
   name='Lossless decompressor',
   handler=function(str,...)
      local stream = zlib.inflate()
      local decompressed_str,eof,bytes_in,bytes_out=stream(str);
      leda.send('data',decompressed_str,...)
   end,
   init="require 'zlib'"
}

utils.quit={
   name='Process term',
   handler=function ()
       leda.quit()
   end
}

--Graph builders

utils.linear_pipeline=function(...)
   local arg={...}
   local g=leda.graph{}

   local last_s=nil

   for i,v in ipairs(arg) do
      assert(type(v)=="function" or leda.is_stage(v),"Argument #"..tostring(i).." must be a function or a stage")
      local stage=leda.stage(v)
      if last_s then g:add(last_s:connect(stage)) 
      else g.start=stage end
      last_s=stage
   end
   return g
end

-- Stage builders

-----------------------------------------------------------------------------
-- Returns a stage that sends 'n' copies of each event it receives to its default output
-- param:   'n':     number of copies to push
-----------------------------------------------------------------------------
utils.get_copy_stage=function (n,cpy_func)
	assert(type(n)=="number" and n>=1,"param #1 must be a number greater than one")
	return leda.stage{
   		handler=function (...)
      		for i=2,n do
        			if cpy_func then leda.send(1,cpy_func(...))
      			else leda.send(1,...) end
      		end
   		   if n >= 1 then
               leda.send(1,...)
   		   end

		end,
   		bind=function(self) 
      		assert(self[1],"Copy must have an output at [1]") 
   		end,
   		name="Event copy ("..tostring(n)..")"
	}
end

--Connector builders

utils.event_recorder=function(file,head,key,tail,meth)
	assert(type(file)=="string","Parameter 'filename' must be a string")
	local handler=function(...)
		local relative_time=leda.gettime()-init_time
		assert(leda.send(key,...))
		--print("Event",relative_time)
		local event=leda.encode({timestamp=relative_time,...})
		out:awrite(#event..event)
	end
	local init=function()
		require "leda.utils.io"
		init_time=leda.gettime()
		out=io.open(file,"w")
	end
	local stage=leda.stage{handler=handler,init=init,name="Event recorder ("..file..')',serial=true}
	
	return {leda.connect(head,key,stage,"local"), leda.connect(stage,key,tail,meth)}
end

local epsilon=0.000002

utils.event_replayer=function(file,timed,key,tail)
	assert(type(file)=="string","Parameter 'filename' must be a string")
	local handler=function()
		local event_size=assert(input:read("*number"))
		while event_size do
			local buf=assert(input:read(event_size))
			local dec=assert(leda.decode(buf))	
			assert(leda.send(key,unpack(dec)))
			local when=dec.timestamp+init_time
			local delay=when-leda.gettime()
			--print('RELATIVE',dec.timestamp,delay)
			if delay > epsilon and timed then
				--print('SLEEPING',delay,now,when,dec.timestamp)
				leda.sleep(delay-epsilon)
			end			
			event_size=input:read("*number")
		end
	end
	local init=function()
		require "leda.utils.io"
		input=assert(io.open(file,"r"))
		init_time=leda.gettime()
	end
	local stage=leda.stage{handler=handler,init=init,name="Event replayer ("..file..")",serial=true}
	stage:send()
	return leda.connect(stage,key,tail,"local")
end

return utils
