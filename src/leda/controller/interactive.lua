-----------------------------------------------------------------------------
-- Leda simple fixed thread pool controller
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

-----------------------------------------------------------------------------
-- Declare module and import dependencies
-----------------------------------------------------------------------------
local base = _G
local dbg = leda.debug.get_debug("Controller: Interactive: ")
local kernel,plot=leda.kernel,require('leda.utils.plot')
local table=table
local default_thread_pool_size=2
local print,loadstring,pcall,os,string,pairs,ipairs,tostring,io,assert=
      print,loadstring,pcall,os,string,pairs,ipairs,tostring,io,assert
local read=io.read
local write=io.write
local stderr=io.stderr
local prompt='> '

module("leda.controller.interactive")

local pool_size=default_thread_pool_size
-----------------------------------------------------------------------------
-- Controller init function
-----------------------------------------------------------------------------

local function readline(prompt)
   write(prompt)
   return read()
end

local function compile(line)
    return loadstring(line,'local')
end

local function evaluate(chunk)
    local ok,err = pcall(chunk)
    if not ok then
        return err
    end
    return nil
end

local function eval_lua(line)
    local chunk,err = compile(line)
     if not err then
        err = evaluate(chunk)
    end
    if err then
        stderr:write(err)
    end
end

local last_t={}
local init_time=kernel.gettime()
local gr=""


function get_init(n)
   return function (g)
   stderr:write("\027[2J")
   pool_size=n
   init_time=kernel.gettime()
   for i=1,n do
      kernel.new_thread()
      dbg("Thread %d created",i)
   end
   
   local fn = os.tmpname()
  
   plot.plot_graph(g,fn..".dot")

   local f=assert(io.popen("graph-easy "..fn..".dot --boxart","r"))
   gr=assert(f:read('*a'))
   f:close()
   os.remove(fn)
--   os.remove(fn..".dot")

   local line = readline(prompt)
      while line do
         if line == 'quit' then 
            stderr:write("Quiting...\n")
            os.exit(0)
         elseif line == '+' then 
            kernel.new_thread() 
            stderr:write("\027[2J")
            stderr:write("Thread created...\n")
         elseif line == '-' then 
            kernel.kill_thread() 
            stderr:write("\027[2J")
            stderr:write("Thread killed...\n")
         else
            stderr:write("\027[2J")
            eval_lua(line)
         end
         stderr:write("\n")
         update(fn)
         stderr:flush()
         line = readline(prompt)
      end
   end
end
init=get_init(default_thread_pool_size)

function update(fn)
   local ps=kernel.thread_pool_size()
   local rs=kernel.ready_queue_size()
   local rc=kernel.ready_queue_capacity()
   local stats=kernel.stats()
   stderr:write("\n")
   stderr:write(gr)
   stderr:write("\n")
   if stats then
   stderr:write(string.format("\nTotal execution time: %.3fs\n",kernel.gettime()-init_time))
   stderr:write(string.format("Thread_pool_size=%d\tReady_queue_size=%d\tReady_queue_capacity=%d\n",ps,rs,rc))
   for k,v in ipairs(stats) do 
      local last=last_t[k] or {0,0}
      last_t[k]={v.events_pushed,kernel.gettime()}
      local l_e=v.events_pushed-last[1]
      local l_t=kernel.gettime()-last[2]
      stderr:write(string.format("%d: name='%s' active=%d events=%d queue_size=%d (%d) executed=%d  latency=%.3fms throughput=%.1fev/s\n",k,tostring(v.name),v.active,v.events_pushed,v.event_queue_size,v.event_queue_capacity,v.times_executed,v.average_latency/1000,l_e/l_t))
   end
   end
end

function get(n)
   return {init=get_init(n),event_pushed=event_pushed,finish=finish}
end
