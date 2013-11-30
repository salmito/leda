--os.execute('lua -l leda -e "print(leda.start{port=8888})"&')
--os.execute('sleep 1')
local _=require 'leda'
s1=_.stage"Stage1"(function() 
   leda.sleep(0.5)
   leda.quit("My process is: ",leda.stage.process()," and my name is:",self.name) end):push()
s1.name='Stage1'

g=_.g'graph'(s1)--,s2)

print(g:run())
print(g:run())
print(g:run())
print(g:run())
