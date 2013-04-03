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

-----------------------------------------------------------------------------
-- Add pending data to the stage
-----------------------------------------------------------------------------    
function index.send(self,...)
   local v={...}
   table.insert(self.pending,v)
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
   return function (g) return g:connect(head,key,tail,method) end
end
t.connect=index.connect

function t.metatable()
   return stage
end

-----------------------------------------------------------------------------
-- Creates a new stage and returns it
-- param:   't': table used to hold the stage representation
-----------------------------------------------------------------------------

-----------------------------------------------------------------------------
-- Creates a new stage and returns it
-- param:   't': table used to hold the stage representation
-----------------------------------------------------------------------------

local function new_stage_t(...)--t,init,name,bind,serial)
   local s={...}
   if type(s[1])=="table" then
      s=s[1]
   end
   if is_stage(s) then
      local ns={}
      for k,v in pairs(s) do
         ns[k]=v
      end
      s=ns
   end
   s.handler=s.handler or s[1]
   s.init=s.init or s[2]
   s.bind=s.bind or s[3]
   s.serial=s.serial or s.stateful or s[4]
   s.autostart=s.autostart or s[5]
   s.pending={}
   
   assert(type(s.handler)=="function" or type(s.handler)=="string","Invalid handler type (function or string expected)")
   assert(type(s.init)=="function" or type(s.init)=="string" or s.init==nil,"Invalid init type (function or string expected)")

   s=setmetatable(s,stage)
   if _ENV then
      local f_handler=s.handler
      if type(f_handler)=="function" then
         local upname,env = debug.getupvalue (s.handler, 1)
         if upname == '_ENV' then
            debug.setupvalue (s.handler, 1,{})
         end
         s.handler=kernel.encode(s.handler)
         s.handler_enc=true
         debug.setupvalue (f_handler, 1,env)
      end
      
      local f_init=s.init
      if type(f_init)=="function" then
         local upname,env = debug.getupvalue (s.init, 1)
         if upname == '_ENV' then
            debug.setupvalue (s.init, 1,{})
         end
         s.init=kernel.encode(s.init)
         s.init_enc=true
         debug.setupvalue (f_init, 1,env)
      end
   end

   if type(s.handler)=="function" then 
      s.handler=kernel.encode(s.handler)
      s.handler_enc=true
   end

   if type(s.init)=="function" then 
      s.init=kernel.encode(s.init)
      s.init_enc=true
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

function index:compose(key,consumer)
   assert(type(key)=='number' or type(key)=='string' or (is_stage(key) and consumer==nil),"Invalid argument #1, only string or number keys are allowed")
   if is_stage(key) and consumer==nil then key,consumer=1,key end
   assert(is_stage(consumer), "Invalid argument, stage expected")
   
   local producer_init=self.init
   local consumer_init=consumer.init
   local consumer_handler=consumer.handler
   
   local function new_init()
      local oldinit=leda.decode(producer_init)
      if type(oldinit)=='string' then assert(loadstring(oldinit)) end
      local consinit=leda.decode(consumer_init)
      if type(consinit)=='string' then assert(loadstring(consinit)) end
      local conshand=leda.decode(consumer_handler)
      if type(conshand)=='string' then assert(loadstring(conshand)) end
      assert(type(conshand)=='function',"Error loading stage handler")
      local f=function(self,...)
         _G.print("AE",self,...)
         conshand(...)
         return true
      end
      leda.output[key].send=f
      if type(oldinit)=='function' then oldinit() end
      if type(consinit)=='function' then consinit() end
      
   end
   self.init=kernel.encode(new_init)
   
   return self
end

return t
