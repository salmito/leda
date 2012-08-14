require "leda.utils"

wait_client=stage{
   handler=function(port)
       local server_sock=socket.bind("*",port)
       print("SERVER: Waiting on port >> ",port)
       local sockfd=leda.socket.wrap(server_sock)
       epfd,err=leda.epoll.create(1)
       local a,err=leda.epoll.add_read(epfd,sockfd)
       while true do
          local sockets,err=leda.epoll.wait(epfd,0);
          if sockets.read[1] then
            local srv=leda.socket.unwrap(sockets.read[1],"tcp{server}")
            local cli_sock=srv:accept()
            local cli=leda.socket.wrap(cli_sock)
            print("SERVER: Sending client",cli)
            leda.send(1,cli)
            leda.socket.wrap(srv)
          end
          leda.nice()
       end
   end, 
   init=function () require "socket" end,
   name="wait client"
}

epoll_reader=stage{
   handler=function(sock)
      epfd,err=leda.epoll.create(10)
      assert(epfd,err)
      local a,err=leda.epoll.add_read(epfd,sock)
      if not a then print(err) end
      while true do
         local sockets,err=leda.epoll.wait(epfd,0);
         if sockets then
            for i=1,#sockets.read do
               local cli=leda.socket.unwrap(sockets.read[i])
               local line,err=cli:receive()
               if not line then 
                  print(err)
                  leda.epoll.remove(epfd,sockets.read[i])
               else 
                  leda.send(1,line) 
                  leda.socket.wrap(cli)
               end
            end
         else print(err) 
         end
         local new_sock=leda.debug.peek_event()
         if new_sock then leda.epoll.add_read(epfd,new_sock) end
         leda.nice()
      end
   end,
   init=function() require "socket" end,
   serial=true
}

local_echo=stage{
   handler=function(line)
      print(line)
   end,
   name="echo request locally"
}

leda.connect(epoll_reader,local_echo)
leda.connect(wait_client,epoll_reader)

g=graph{wait_client,epoll_reader,local_echo}


wait_client:send(9999)
g:run(leda.controller.fixed_thread_pool.get(1))
