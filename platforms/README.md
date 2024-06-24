# Host/VM interaction API

Sloth is designed to allow easy interoperation between host and Sloth.

## Stack API

push(v) -> pushes a value to the stack
pop() -> pops a value from the stack
pick(a) -> picks the value at TOP - a
place(a, v) -> places a value v at TOP - a

rpush(v) -> pushes an address to the return stack
rpop() -> pops an address from the return stack

## Execution API

eval(q) -> executes quotation/primitive q inside the VM
do([...]) -> executes host functions chained
