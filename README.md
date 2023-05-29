@mainpage
# coco
coco is a tiny, portable cooperative scheduler for C. It can be used to as a simple coroutine library, or with the capabilities of a small operating system.

### coco features:
<h4>coco core (coco.h)</h4>

- Task concurency
- Dynamic task creation
- Task contexts for data restoration upon reentry
- Yield points and yields for a time period
- Ability for subroutines of tasks to have the ability to yield
- Task exit status
- Task reaping to obtain exit status and check aliveness
- 100% Completely portable (only requires setjmp.h and time.h from the stdlib)
- No dynamic memory needed
- No parallelism, so thread saftey primatives are irrelavent
#### signals
- Inspired by UNIX-style signals
- Default signal handlers
- Install custom signal actions
- Ability to stop (freeze) and continue tasks dynamically with SIGSTP and SIGCONT
#### Go-style channels and waitgroups
- channels provide a FIFO queues for inter-task communication
- waitgroups provide a mechanism for a task to wait on spawned children
