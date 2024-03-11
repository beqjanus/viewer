-- Exercise the Queue, WaitQueue, ErrorQueue family

Queue = require('Queue')
WaitQueue = require('WaitQueue')
ErrorQueue = require('ErrorQueue')
util = require('util')

inspect = require('inspect')

-- ------------------ Queue variables are instance-specific ------------------
q1 = Queue:new()
q2 = Queue:new()

q1:Enqueue(17)

assert(not q1:IsEmpty())
assert(q2:IsEmpty())
assert(q1:Dequeue() == 17)
assert(q1:Dequeue() == nil)
assert(q2:Dequeue() == nil)

-- ----------------------------- test WaitQueue ------------------------------
q1 = WaitQueue:new()
q2 = WaitQueue:new()
result = {}
values = { 1, 1, 2, 3, 5, 8, 13, 21 }

for i, value in pairs(values) do
    q1:Enqueue(value)
end
-- close() while not empty tests that queue drains before reporting done
q1:close()

-- ensure that WaitQueue instance variables are in fact independent
assert(q2:IsEmpty())

-- consumer() coroutine to pull from the passed q until closed
function consumer(desc, q)
    print(string.format('consumer(%s) start', desc))
    local value = q:Dequeue()
    while value ~= nil do
        print(string.format('consumer(%s) got %q', desc, value))
        table.insert(result, value)
        value = q:Dequeue()
    end
    print(string.format('consumer(%s) done', desc))
end

-- run two consumers
coa = coroutine.create(consumer)
cob = coroutine.create(consumer)
-- Since consumer() doesn't yield while it can still retrieve values,
-- consumer(a) will dequeue all values from q1 and return when done.
coroutine.resume(coa, 'a', q1)
-- consumer(b) will wake up to find the queue empty and closed.
coroutine.resume(cob, 'b', q1)
coroutine.close(coa)
coroutine.close(cob)

print('values:', inspect(values))
print('result:', inspect(result))

assert(util.equal(values, result))

-- try incrementally enqueueing values
q3 = WaitQueue:new()
result = {}
values = { 'This', 'is', 'a', 'test', 'script' }

coros = {}
for _, name in {'a', 'b'} do
    local coro = coroutine.create(consumer)
    table.insert(coros, coro)
    -- Resuming both coroutines should leave them both waiting for a queue item.
    coroutine.resume(coro, name, q3)
end

for _, s in pairs(values) do
    print(string.format('Enqueue(%q)', s))
    q3:Enqueue(s)
end
q3:close()

function joinall(coros)
    local running
    local errors = 0
    repeat
        running = false
        for i, coro in pairs(coros) do
            if coroutine.status(coro) == 'suspended' then
                running = true
                local ok, message = coroutine.resume(coro)
                if not ok then
                    print('*** ' .. message)
                    errors += 1
                end
                if coroutine.status(coro) == 'dead' then
                    coros[i] = nil
                end
            end
        end
    until not running
    return errors
end

joinall(coros)

print(string.format('%q', util.join(result, ' ')))
assert(util.equal(values, result))

-- ----------------------------- test ErrorQueue -----------------------------
q4 = ErrorQueue:new()
result = {}
values = { 'This', 'is', 'a', 'test', 'script' }

coros = {}
for _, name in {'a', 'b'} do
    local coro = coroutine.create(consumer)
    table.insert(coros, coro)
    -- Resuming both coroutines should leave them both waiting for a queue item.
    coroutine.resume(coro, name, q4)
end

for i = 1, 4 do
    print(string.format('Enqueue(%q)', values[i]))
    q4:Enqueue(values[i])
end
q4:Error('something went wrong')

assert(joinall(coros) == 2)
