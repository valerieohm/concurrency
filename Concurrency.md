**volatile**
: Signals to compiler that the variable may be changed by some external source, so don't optimize it away.

**RAII**
: **R**esource **A**cquisition **I**s **I**nitialization.  Classes that manage their own resources.

**std::unique_lock**
: Allows manual lock/unlock

**std::scoped_lock**
: Locks multiple mutexes atomically

**std::lock_guard**
: Releases mutex on loss of scope/destruction

deadlock
: mutual exclusion, resource holding, non-preemption, circular wait

resource starvation
:

livelock
:

mutex
: bound to the thread that locks it.

semaphore
: not bound to thread. primitive condition variable






