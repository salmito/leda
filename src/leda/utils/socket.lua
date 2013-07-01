require "socket"

local block=block or false

local tcp_client_mt,err=leda.getmetatable("tcp{client}")
if tcp_client_mt then
   tcp_client_mt.__wrap=function(sock)
      local sockfd=sock:getfd()
      sock:setfd(-1) --do not collect on local instance
      sock:close()
      return function()
	require 'leda.utils.socket'
         local client_mt,err=leda.getmetatable("tcp{client}")
         if err then
            return nil,err
         end
         local container_sock=assert(socket.tcp()) --luasock is required
	 container_sock:close()
         container_sock:setfd(sockfd)
         return leda.setmetatable(container_sock,client_mt)     
      end
   end

   tcp_client_mt.__persist=function(sock)
      error('Unable to send socket "'..tostring(sock)..'" to other processes')
   end


   if not block then
      local old_send=tcp_client_mt.__index.send
      tcp_client_mt.__index.send=function(sock,...)
         leda.aio.wait_io(sock:getfd(),2)
         return old_send(sock,...)
      end
   
      local old_receive=tcp_client_mt.__index.receive
      tcp_client_mt.__index.receive=function(sock,...)
         sock:settimeout(0.0)
         local data = old_receive(sock,...)
         if data then
            return data
         end
         leda.aio.wait_io(sock:getfd(),1)
         return old_receive(sock,...)
      end
   end
end

local tcp_master_mt,err=leda.getmetatable("tcp{master}")
if tcp_master_mt then
   tcp_master_mt.__wrap=function(sock)
      local sockfd=sock:getfd()
      sock:setfd(-1) --do not collect on local instance
      sock:close()
      return function()
         local client_mt,err=leda.getmetatable("tcp{master}")
         if err then
            return nil,err
         end
         local container_sock=socket.tcp() --luasock is required
         container_sock:setfd(sockfd)
         return leda.setmetatable(container_sock,client_mt)     
      end
   end
end

local tcp_server_mt,err=leda.getmetatable("tcp{server}")
if tcp_server_mt then
   tcp_server_mt.__wrap=function(sock)
      local sockfd=sock:getfd()
      sock:setfd(-1) --do not collect on local instance
      sock:close()
      return function()
         local client_mt,err=leda.getmetatable("tcp{server}")
         if err then
            return nil,err
         end
         local container_sock=socket.tcp() --luasock is required
         container_sock:setfd(sockfd)
         return leda.setmetatable(container_sock,client_mt)     
      end
   end
   if not block then   
      local old_accept=tcp_server_mt.__index.accept
      tcp_server_mt.__index.accept=function(sock,...)
         leda.aio.wait_io(sock:getfd(),1)
         return old_accept(sock,...)
      end
   end
end   

local udp_connected_mt,err=leda.getmetatable("udp{connected}")
if udp_connected_mt then
   udp_connected_mt.__wrap=function(sock)
      local sockfd=sock:getfd()
      sock:setfd(-1) --do not collect on local instance
      sock:close()
      return function()
         local connected_mt,err=leda.getmetatable("udp{connected}")
         if err then
            return nil,err
         end
         local container_sock=socket.udp() --luasock is required
         container_sock:setfd(sockfd)
         return leda.setmetatable(container_sock,connected_mt)     
      end
   end
   
   if not block then
     local old_send=udp_connected_mt.__index.send
     udp_connected_mt.__index.send=function(sock,...)
        leda.aio.wait_io(sock:getfd(),2)
        return old_send(sock,...)
     end
     
     local old_receive=udp_connected_mt.__index.receive
     udp_connected_mt.__index.receive=function(sock,...)
        sock:settimeout(0.0)
        local data = old_receive(sock,...)
        if data then return data end
        leda.aio.wait_io(sock:getfd(),1)
        return old_receive(sock,...)
     end
   end
end

local udp_unconnected_mt,err=leda.getmetatable("udp{unconnected}")
if udp_unconnected_mt then
   udp_unconnected_mt.__wrap=function(sock)
      local sockfd=sock:getfd()
      sock:setfd(-1) --do not collect on local instance
      sock:close()
      return function()
         local unconnected_mt,err=leda.getmetatable("udp{unconnected}")
         if err then
            return nil,err
         end
         local container_sock=socket.udp() --luasock is required
         container_sock:setfd(sockfd)
         return leda.setmetatable(container_sock,unconnected_mt)     
      end
   end
   
   local old_sendto=udp_unconnected_mt.__index.sendto
   udp_unconnected_mt.__index.sendto=function(sock,...)
      leda.aio.wait_io(sock:getfd(),2)
      return old_sendto(sock,...)
   end
   
   local old_receive=udp_unconnected_mt.__index.receive
   udp_unconnected_mt.__index.receive=function(sock,...)
      sock:settimeout(0.0)
      local data = old_receive(sock,...)
      if data then return data end
      leda.aio.wait_io(sock:getfd(),1)
      return old_receive(sock,...)
   end
   
   local old_receivefrom=udp_unconnected_mt.__index.receivefrom
   udp_unconnected_mt.__index.receivefrom=function(sock,...)
      sock:settimeout(0.0)
      local data = old_receivefrom(sock,...)
      if data then return data end
      leda.aio.wait_io(sock:getfd(),1)
      return old_receivefrom(sock,...)
   end
end

