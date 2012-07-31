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

read_request=stage{
   handler=function(cli_sock)
      print("SERVER: Serving client",cli_sock)
      local cli=leda.socket.unwrap(cli_sock)
      cli:send("Welcome stranger\r\n")
      local line = cli:receive()
      local last_line=""
      local i=0
      
      while line do
         if (line == "\r" or line=="") and (last_line == "\r" or last_line=="") then 
            break 
         end
         last_line=line
         leda.get_output():send(line)
         i=i+1
         if i==10 then break end
         line = cli:receive()
      end
   
      print("SERVER: Closing connection")
      cli:close()
   end,
      init=function () require "socket" end,
   name="read request from client sock"
}

local_echo=stage{
   handler=function(line)
      print(line)
   end,
   name="echo request locally"
}

leda.insert_before(read_request,local_echo)
leda.insert_before(wait_client,read_request)

g=graph{wait_client,read_request,local_echo}


wait_client:send(9999)
g:run(leda.controller.fixed_thread_pool.get(10))
