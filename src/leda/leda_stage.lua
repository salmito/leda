-----------------------------------------------------------------------------
-- Leda Stage Lua API
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------

-----------------------------------------------------------------------------
-- Declare module and import dependencies
-----------------------------------------------------------------------------
local base = _G
--local pairs,assert,tostring,setmetatable,getmetatable,error,io,print,loadstring=
--      pairs,assert,tostring,setmetatable,getmetatable,error,io,print,loadstring
local string,table,kernel= string,table,leda.kernel
local leda_connector = require("leda.leda_connector")
local is_connector=leda_connector.is_connector
local new_connector=leda_connector.new_connector
local dbug=require("leda.debug")
local dbg = dbug.get_debug("Stage: ")
local dump = string.dump

--local leda=leda

--module("leda.leda_stage")
local t={}

t.stages = {}

----------------------------------------------------------------------------
-- Stage metatable
-----------------------------------------------------------------------------
local stage = {__index = {}}

-----------------------------------------------------------------------------
-- Stage __tostring metamethod
-----------------------------------------------------------------------------
function stage.__tostring (s) 
   if s.name then 
      return string.format("%s (%s)",s.name,kernel.to_pointer(s))
   else
      return string.format("Stage (%s)",kernel.to_pointer(s)) 
   end
end

-----------------------------------------------------------------------------
-- Stage __index metamethod
-----------------------------------------------------------------------------
local index=stage.__index
-----------------------------------------------------------------------------
-- Verify if parameter 's' is a stage
-- (i.e. has the stage metatable)
--
-- returns:       'true' if 's' is a stage
--                'false' if not
-----------------------------------------------------------------------------
local function is_stage(s)
   if getmetatable(s)==stage then return true end
   return false
end
t.is_stage = is_stage

--concat metamethod
function stage.__concat(s1,s2)
	assert(is_stage(s1),"Invalid parameter #1, (stage expected, got "..type(s1))
	assert(is_stage(s2),"Invalid parameter #2, (stage expected, got "..type(s2))
	return s1:connect(s2)
end

--concat metamethod
function stage.__call(s1,p)
	assert(is_stage(s1),"Invalid parameter #1, (stage expected, got "..type(s1))
	return setmetatable({},{__concat=function(t,s2) return s1:connect(p,s2) end})
end


-----------------------------------------------------------------------------
-- Add pending data to the stage
-----------------------------------------------------------------------------    
function index.send(self,...)
   local v={...}
   table.insert(self.pending,v)
   return self
end

-----------------------------------------------------------------------------
-- method stage:connect([key,]tail,method)
-- Connect two stages at output port 'key', if key is absent, assume it is the 
-- default output (1)
-- the 'tail' argument must be a stage.
-- the method is a function called when the producer stage calls the send
-- method on the new connector output port
-----------------------------------------------------------------------------
function index.connect(head,key,tail,method)
   return setmetatable({head=head},{__index={
   	run=function(...) 
   		return leda.graph(function(g) return g:connect(head,key,tail,method) end):run(...)
   	end
   },
   __call=function(g) return g:connect(head,key,tail,method) end})
   --function (g) return g:connect(head,key,tail,method) end
end
t.connect=index.connect

function t.metatable()
   return stage
end

local function strip_upvalues(f)
   while true do
  		local upname, v = debug.getupvalue(f, i)
  		if upname == '_ENV' then
       	debug.setupvalue (f_handler, i,{})
      elseif upname == 'leda' then
        	debug.setupvalue (f_handler, i,nil)
      end
  		if not n then break end
  		i = i + 1
    end
end

-----------------------------------------------------------------------------
-- Creates a new stage and returns it
-- param:   't': table used to hold the stage representation
-----------------------------------------------------------------------------

local function new_stage_t(...)--t,init,name,bind,serial)
   local s={...}
   if type(s[1])=="table" then
      s=s[1]
   elseif type(s[1])=="function" or type(s[1])=='string' then
   	s.handler=s[1]
   	if select('#',...) > 1 then
	   	s.autostart={select(2,...)}   	
   	end
   	table.remove(s,1)
   end
   if is_stage(s) then
      local ns={}
      for k,v in pairs(s) do
         ns[k]=v
      end
      s=ns
   end
   s.serial=s.serial or s.stateful
   s.pending={}
   assert(type(s.handler)=="function" or type(s.handler)=="string","Invalid handler type (function or string expected)")
   assert(type(s.init)=="function" or type(s.init)=="string" or s.init==nil,"Invalid init type (function or string expected)")

   s=setmetatable(s,stage)
--   if _ENV then
      local f_handler=s.handler
      if type(f_handler)=="function" then
      	local env,envi,i=nil,nil,1
         while true do
        		local upname, v = debug.getupvalue(f_handler, i)
        		if upname == '_ENV' then
        			env,envi=v,i
            	debug.setupvalue (f_handler, i,{})
            elseif upname == 'leda' then
					error("cannot use 'leda' as upvalue of function")
	         end
        		if not upname then break end
        		i = i + 1
	      end   
         s.handler=kernel.encode(f_handler)
         s.handler_enc=true
         if env then
	         debug.setupvalue (f_handler, envi, env)
	      end
      end
      
      local f_init=s.init
      if type(f_init)=="function" then
      	local env,envi,i=nil,nil,1
         while true do
        		local upname, v = debug.getupvalue(f_init, i)
        		if upname == '_ENV' then
        			env,envi=v,i
        			debug.setupvalue (f_init, i,{})
            elseif upname == 'leda' then
					error("cannot use 'leda' as upvalue of function")
	         end
        		if not upname then break end
        		i = i + 1
	      end   
         s.init=kernel.encode(f_init)
         s.init_enc=true
         if env then
	         debug.setupvalue (f_init, envi,env)
	      end
      end
--   end

--[[   if type(s.handler)=="function" then 
      s.handler=kernel.encode(s.handler)
      s.handler_enc=true
   end

   if type(s.init)=="function" then 
      s.init=kernel.encode(s.init)
      s.init_enc=true
   end--]]

	assert(type(s.handler)=='string',"Handler type error")
	if s.init then
		assert(type(s.init)=='string',"Init type error")
	end
   
   if not s.init_enc then
      s.init=kernel.encode(s.init)
      s.init_enc=true   
   end

   if not s.handler_enc then
      s.handler=kernel.encode(s.handler)
      s.handler_enc=true 
   end

   
   s.name=s.name or tostring(s)
   s.pending={}
   table.insert(t.stages,s)
--   for k,v in pairs(s) do print(k,v) end
   
   return s
end

function t.new_stage(...) 
   local s={...}
   if type(s[1])=='string' and #s==1 then -- stage "name" {...}
      local name=s[1]
      return function(...) local s=new_stage_t(...) s.name=name return s end
   end
   return new_stage_t(...)
end

return t
