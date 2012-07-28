require "leda.utils"

local kernel=leda.kernel

print(kernel.ready_queue_size(), kernel.pool_size())

local th={}

for i=1,10 do
   table.insert(th,kernel.new_thread())
end

print(kernel.ready_queue_size(), kernel.pool_size())

for i=1,10 do
   th[i]:kill()
end

kernel.sleep(1)

print(kernel.ready_queue_size(), kernel.pool_size())

