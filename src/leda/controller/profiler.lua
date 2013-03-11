-----------------------------------------------------------------------------
-- Leda simple fixed thread pool controller
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

-----------------------------------------------------------------------------
-- Declare module and import dependencies
-----------------------------------------------------------------------------
local leda=leda or require "leda"
local kernel=require "leda.kernel"
local dbg = leda.debug.get_debug("Controller: Profiler: ")
local print,loadstring,pcall,os,string,pairs,ipairs,tostring,io,assert,table=
      print,loadstring,pcall,os,string,pairs,ipairs,tostring,io,assert,table
local output=io.stderr
local default_thread_pool_size=kernel.cpu()
local init_time=nil
local t={}
local th={}
local last_t={}
local last_t2={}
local testcase=testcase or 'default'

-----------------------------------------------------------------------------
-- Controller init function
-----------------------------------------------------------------------------
local resolution=profiler_resolution or 0.1
local stages,connectors=nil,nil
local graph=nil

function t.init(g)
   graph=g
   if maxpar then
      kernel.maxpar(maxpar)
   end
   for i=1,(thread_pool_size or default_thread_pool_size) do
      table.insert(th,kernel.thread_new())
      dbg("Thread %d created",i)
   end
   if profile_output then
      output=io.open(profile_output,"w")
   end
   stages,connectors=kernel.stats()
   output:write('testcase\ttype\ttime\tid\tname\tready\tthreads\tmax_par\tevents\tqueue_size\texecuted\terrors\tlatency\tthroughput\tactive_threads\n')
   kernel.add_timer(resolution,1)
   init_time=kernel.gettime()
end

local iit=0

local out_temp={}
function t.on_timer(id)
   local now=leda.gettime()-init_time
   local ps=kernel.thread_pool_size()
   local rs,rsc=0,kernel.ready_queue_size()
   local ta=0
   local out={}
   if rsc<0 then rs=0 ta=ps+rsc else ta=ps rs=rsc end
   local rc=kernel.ready_queue_capacity()
   local stats,cstats=kernel.stats()
   
   
--   for k,v in ipairs(stats) do
--      if v.events_pushed>0 or v.times_executed>0 then
   
   for cl in pairs(graph:clusters()) do
      if cl:is_local(leda.process.get_localhost(),leda.process.get_localport()) then
         for s in pairs(cl) do
            if s~="process_addr" then
            local k=graph:getid(s)+1
            local v=stats[tonumber(graph:getid(s))+1]
            
         local last=last_t[k] or {0,0}
         last_t[k]={v.events_pushed,kernel.gettime()}
         local l_e=v.events_pushed-last[1]
         local l_t=kernel.gettime()-last[2]
         table.insert(out,
            string.format("%s\tstage\t%.1f\t%d\t%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%.6f\t%.6f\t%d\n",
            testcase,
            now,
            k-1,
            tostring(v.name), 
            v.active,
            ps,
            v.maxpar, 
            v.events_pushed, 
            v.event_queue_size, 
            v.times_executed, 
            v.errors, 
            v.average_latency, 
            l_e/l_t,ta
            ))
           end     
         end
      end
   end
   for k,v in ipairs(cstats) do
      if  v.events_pushed > 0 then
         local last=last_t2[k] or {0,0}
         last_t2[k]={v.events_pushed,kernel.gettime()}
         local l_e=v.events_pushed-last[1]
         local l_t=kernel.gettime()-last[2]
         table.insert(out,
         string.format("%s\tconnector\t%.1f\t%d\t%s.%s->%s\t0\t%d\t0\t%d\t0\t0\t0\t%.6f\t%.6f\t%d\n",
         testcase,
         now,
         k-1, 
         tostring(v.producer), 
         tostring(v.key),
         tostring(v.consumer),
         ps,
         v.events_pushed,
         v.average_latency,
         l_e/l_t,
         ta
         ))
      end
   end
   table.insert(out_temp,table.concat(out))
   output:write(table.concat(out_temp))
   out_temp={}
         kernel.stats_latency_reset()
   iit=iit+1
   if iit >= 10 then
--      kernel.stats_latency_reset()
      iit=0
--      last_t={}
--      last_t2={}
   end
end

--[[


local last_latency_log={}
local last_sample_time=0
local prebuffer=1
local last_event_log={}
local num_events={con={},sta={}}
local last_events={con={},sta={}}
  
function t.on_timer(id)
   print('ae')
   if #last_latency_log > prebuffer then
      output:write(table.concat(last_latency_log))
      last_latency_log={}
   end
   if #last_event_log > prebuffer then
      output:write(table.concat(last_event_log))
      last_event_log={}
   end
   local now=leda.gettime()-init_time
   if last_sample_time > 0 then
      local passed=leda.gettime()-last_sample_time
      for s,v in pairs(num_events.sta) do
         local tot=v - (last_events.sta[s] or 0)
         if tot > 0 then
            local tp=tot/passed
            output:write(
               string.format("%s\tstage_throughput\t%.6f\t%s\t%.6f\n",testcase,now,stages[s].name,tp)
            )
         end
         last_events.sta[s]=v
      end
      for c,v in pairs(num_events.con) do
         local tot=v - (last_events.con[c] or 0)
         if tot > 0 then
            local tp=tot/passed
            output:write(
               string.format("%s\tstage_throughput\t%.6f\t%s.%s->%s\t%.6f\n",testcase,now,connectors[c].producer,connectors[c].key,connectors[c].consumer,tp)
            )
         end
         last_events.con[c]=v
      end
   end
   last_sample_time=leda.gettime()   
end

function t.on_release(id,time)
--   print('release')
--   local now=leda.gettime()-init_time
--   last_latency_log[#last_latency_log+1]=
--   string.format("%s\tstage_latency\t%.6f\t%s\t%.6f\n",testcase,now,stages[id].name,time/1000000)
end

function t.on_event(id,events,c_id,c_ev,time)
--   print('event')
--   local now=leda.gettime()-init_time
--   last_event_log[#last_event_log+1]=
--   string.format("%s\tcommunication\t%.6f\t%s.%s->%s\t%.6f\n",testcase,now,connectors[c_id].producer,connectors[c_id].key,connectors[c_id].consumer,time/1000000)
--   num_events.con[c_id]=c_ev
--   num_events.sta[id]=events
end

--function t.on_error(id)
 --  local s=kernel.stats(id)
 --  print('Error on stage',id,s[id].name,s[id].errors)
--end
--]]
-----------------------------------------------------------------------------
-- Controller finish function
-----------------------------------------------------------------------------
function t.finish()
   output:close()
end
leda.controller=leda.controller or {}
leda.controller.profiler=t
return t
