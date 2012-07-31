require "leda.utils"

wait_client=stage{
   handler=function(port)
       local server_sock=socket.bind("*",port)
       print("SERVER: Waiting on port >> ",port)
       while true do
          local cli_sock=server_sock:accept()
          local cli=leda.socket.wrap(cli_sock)
          print("SERVER: Sending client",cli)
          leda.get_output():send(cli)
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
         local sockets,err=leda.epoll.wait(epfd,1);
         if sockets then
            for i=1,#sockets.read do
               local cli=leda.socket.unwrap(sockets.read[i])
               local line,err=cli:receive()
               if not line then 
                  print(err)
                  leda.epoll.remove(epfd,sockets.read[i])
               else 
                  leda.get_output():send(line) 
                  leda.socket.wrap(cli)
               end
            end
         else print(err) 
         end
         local new_sock=leda.peek_event()
         if new_sock then leda.epoll.add_read(epfd,new_sock) end
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

leda.insert_before(epoll_reader,local_echo)
leda.insert_before(wait_client,epoll_reader)

g=graph{wait_client,epoll_reader,local_echo}


wait_client:send(9999)
g:run()
