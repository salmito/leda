require "leda"
require "leda.controller.fixed_thread_pool"

local stage,graph=leda.stage,leda.graph

wait_client=stage{
   handler=function(port)
       local server_sock=socket.bind("*",port)
       print("SERVER: Waiting on port >> ",port)
       while true do
          local cli_sock=server_sock:accept()
          print("SERVER: Sending client",cli)
          leda.send('Client socket',cli_sock)
       end
   end, 
   init=function () 
      require "leda.utils.socket" 
   end,
   bind=function (out)
      assert(out['Client socket'],"'Client socket' port bust be connected")
      assert(out['Client socket'].type==leda.couple,"Stages '"..
             tostring(out['Client socket'].producer).."' and '"..tostring(out['Client socket'].consumer)..
             "' must be coupled")
   end,
   name="wait client"
}

read_request=stage{
   handler=function(cli_sock)
      print("SERVER: Serving client",cli_sock)
      local cli=cli_sock
--      assert(leda.sleep(1))
      cli:send("Welcome stranger\r\n")
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
wait_client:connect("Client socket",read_request,leda.couple)
}

wait_client:send(43)

--g:part(wait_client+read_request,local_echo):map("localhost","localhost:7777")
--g:plot()
g:run{controller=leda.controller.fixed_thread_pool.get(1)}
