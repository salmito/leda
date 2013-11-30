local leda=require "leda"

local stage,graph=leda.stage,leda.graph

wait_client=stage{
   handler=function(port)
       local socket=require "leda.utils.socket" 
       local server_sock=assert(socket.bind("*",port))
       print("SERVER: Waiting on port >> ",port,server_sock)
       while true do
          local cli_sock=server_sock:accept()
          print("SERVER: Sending client",getmetatable(cli_sock).__wrap)
          assert(leda.send('Client socket',cli_sock))
       end
   end, 
   bind=function (self,out)
      assert(out['Client socket'],"'Client socket' port bust be connected")
   end,
   name="wait client"
}:push(port or "4000")

read_request=stage{
   handler=function(cli_sock)
      print("SERVER: Serving client",cli_sock)
      local cli=cli_sock
      leda.sleep(1)
      cli:send("Welcome stranger, hit enter twice to close connection\r\n")
      local line = cli:receive()
      local last_line=""
      local i=0
      
      while line do
         if (line == "\r" or line=="") and (last_line == "\r" or last_line=="") then 
            break 
         end
         last_line=line
         leda.send('Line sent',line)
         i=i+1
--         if i==10 then break end
         line = cli:receive()
      end
   
      print("SERVER: Closing connection")
      cli:close()
   end,
      init=function () require "leda.utils.socket"  end,
   name="read request from client sock"
}

local_echo=stage{
   handler=function(line)
      print(line)
   end,
   name="echo request locally"
}

g=graph{read_request:connect("Line sent",local_echo),
wait_client:connect("Client socket",read_request)
}

g:run{controller=leda.controller.singlethread}
