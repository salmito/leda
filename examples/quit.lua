--os.execute('lua -l leda -e "print(leda.start{port=8888})"&')
--os.execute('sleep 1')
local _=require 'leda'
s1=_.s"Stage1"(function() 
   leda.sleep(0.5)
   leda.quit("My process is: ",leda.stage.process()," and my name is:",self.name) end)
--s2=_.s"Stage2"(s1)

s1.autostart=true
--s2.autostart=true

g=_.g'graph'(s1)--,s2)

--g:part(s1,s2):map('localhost:9999','localhost:8888')

print(g:run())
print(g:run())
print(g:run())
print(g:run())
