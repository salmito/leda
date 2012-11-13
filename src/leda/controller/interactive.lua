-----------------------------------------------------------------------------
-- Leda simple fixed thread pool controller
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

-----------------------------------------------------------------------------
-- Declare module and import dependencies
-----------------------------------------------------------------------------
local base = _G
local dbg = leda.debug.get_debug("Controller: Interactive: ")
local plot=pcall(require,'leda.utils.plot')
local table,leda=table,leda
local print,loadstring,pcall,os,string,pairs,ipairs,tostring,io,assert=
      print,loadstring,pcall,os,string,pairs,ipairs,tostring,io,assert
local read=io.read
local write=io.write
local stderr=io.stderr
local prompt="leda-"..leda._VERSION..'> '
kernel=leda.kernel
local kernel=kernel
leda.send=kernel.send
local default_thread_pool_size=kernel.cpu()

module("leda.controller.interactive")

local pool_size=default_thread_pool_size
-----------------------------------------------------------------------------
-- Controller init function
-----------------------------------------------------------------------------

local function readline(prompt)
   stderr:write(prompt)
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
local last_t2={}
local init_time=kernel.gettime()
local gr=""
local th={}

function get_init(n)
   return function (g)
--   stderr:write("\027[2J")
   pool_size=n or pool_size
   init_time=kernel.gettime()
   for i=1,n do
      table.insert(th,kernel.new_thread())
      dbg("Thread %d created",i)
   end
   
   local fn = os.tmpname()
  
   if plot and leda.utils.plot then
         leda.utils.plot.plot_graph(g,fn..".dot") 
   end

   local f=assert(io.popen("graph-easy "..fn..".dot --boxart 2>/dev/null","r"))
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
         table.insert(th,kernel.new_thread())
--         stderr:write("\027[2J")
         stderr:write("Thread created...\n")
      elseif line == '-' then 
         kernel.kill_thread() 
--            stderr:write("\027[2J")
         stderr:write("Thread killed...\n")
      elseif line~="" then
--          stderr:write("\027[2J")
         eval_lua(line)
         stderr:write("\n")
      else
         update(fn)
      end
      stderr:flush()
      line = readline(prompt)
      end
   end
end
init=get_init(default_thread_pool_size)

function update(fn)
   local ps=kernel.thread_pool_size()
   local rs,rsc=0,kernel.ready_queue_size()
   local ta=0
   if rsc<0 then rs=0 ta=ps+rsc else ta=ps rs=rsc end
   local rc=kernel.ready_queue_capacity()
   local stats,cstats=kernel.stats()
   stderr:write(gr)
   stderr:write("\n")
   if stats then
   stderr:write(string.format("\nTotal execution time: %.3fs\n",kernel.gettime()-init_time))
   stderr:write(string.format("Thread_pool_size=%d (%d active)\tReady_queue_size=%d\tReady_queue_capacity=%d\n",ps,ta,rs,rc))
   stderr:write("===== Stages =====\n")
   for k,v in ipairs(stats) do 
      if v.events_pushed>0 or v.times_executed>0 then
         local last=last_t[k] or {0,0}
         last_t[k]={v.events_pushed,kernel.gettime()}
         local l_e=v.events_pushed-last[1]
         local l_t=kernel.gettime()-last[2]
         stderr:write(string.format("%d: name='%s' ready=%d maxpar=%d events=%d queue_size=%d executed=%d errors=%d latency=%.3fms throughput=%.1fev/s\n",
         k-1,tostring(v.name), v.active, v.maxpar, v.events_pushed, v.event_queue_size, v.times_executed, v.errors, v.average_latency/1000, l_e/l_t))
      end
   end
   stderr:write("===== Connectors =====\n")
   for k,v in ipairs(cstats) do
      if  v.events_pushed > 0 then
         local last=last_t2[k] or {0,0}
         last_t2[k]={v.events_pushed,kernel.gettime()}
         local l_e=v.events_pushed-last[1]
         local l_t=kernel.gettime()-last[2]
         stderr:write(string.format("%d: name='%s.%s -> %s' events=%d latency=%.3fms throughput=%.1fev/s\n",k-1, tostring(v.producer), tostring(v.key), tostring(v.consumer) ,v.events_pushed,v.average_latency/1000,l_e/l_t))
      end
   end
   end
end

function get(n)
   return {init=get_init(n),event_pushed=event_pushed,finish=finish}
end
