require "socket" 

function f2(i,init,N,M)
      if i==N then
         print(3,"Thread_based",N,M,socket.gettime()-init)
      end
end


function test(N,M)
   local init=socket.gettime()
   par={}
   for j=0,M do par[j]=1 end
   for i=1,N do
      f2(i,init,N,M,unpack(par))
   end
end

function f() 
      test(100,10)
      test(1000,10)
      test(10000,10)
      test(100000,10)
      test(1000000,10)
      test(10000000,10)         

      test(100,100)
      test(1000,100)
      test(10000,100)
      test(100000,100)
      test(1000000,100)
      test(10000000,100)         

      test(100,1000)
      test(1000,1000)
      test(10000,1000)
      test(100000,1000)
      test(1000000,1000)
    --  test(10000000,1000)         

end

f()
