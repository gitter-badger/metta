## Previous works

### PEBBLE

Key Ideas: Protection Domains, Threads and Portals

 * **Minimal supervisor-mode nucleus**, responsible for little more than context switches.
   - Most functionality is provided by servers that execute in user mode without special privileges.
   - The kernel is responsible only for switching between protection domains.
   - If something can be run at user level, it is.
   - Pebble kernel is much smaller than contemporary kernels (Unix, Inferno, L4).

    Pebble nucleus maintains only a handful of data structures:
    each thread associated with a thread_t has a:
    - pointer to the thread's current portal_table
    - user stack is used normally
    - interrupt stack is used when interrupt or exception occurs
    - invocation stack keeps track of portal traversals and processor state. The portal call code saves the invoking domain’s state on this stack. It also saves the address of the corresponding return portal on the invocation stack. The portal return code restores the state from this stack.

 * Safe extensibility: the system is constructed from untrusted components and reconfigured while running.
   - Software components execute in separate protection domains enforced by hardware (MMU).
   - Typical Pebble components:.
    * CPU scheduler,
    * device drivers (including interrupt handling),
    * VM,
    * file systems.

 * Interrupt latency is low and predictable by design.

 * A protection domain consists of the set of memory pages and the set of portals accessible to threads
   executing in that domain. Protection domains may share memory and portals.

 * A thread is a scheduleable entity that executes inside a protection domain and may switch to another
   domain by portal traversal.

   Threads cannot block anywhere except inside the scheduler. Synchronization primitives are managed
   entirely by the user-level scheduler. When a thread running in a protection domain creates a semaphore,
   two portals that invoke the scheduler (for P and V operations) are added to the protection domain’s portal table.
   The thread invokes P in order to acquire the semaphore. If the P succeeds, the scheduler grants the calling protection
   domain the semaphore and returns. If the semaphore is held by another protection domain, the P fails, the
   scheduler marks the thread as blocked, and then schedules another thread. A V operation works analogously; if
   the operation unblocks a thread that has higher priority than the invoker, the scheduler can block the invoking
   thread and run the newly-awakened one.

 * Inter-domain communication is performed by portals which are generated dynamically.
   - portals transfer the calling thread to another protection domain,
   - portal traversal is a generalization of hardware interrupt handling,
   - portal code is generated dynamically and performs portal-specific actions,
   - portal code is trusted and managed by the portal manager,
   - portal code runs in kernel mode and may access or modify private data structures belonging to certain servers (e.g. VM),
   - portal may perform additional actions (parameter modification, virtual memory, etc.),
   - portal code may never block and may not contain loops,
   - portals are generally *created in pairs* for each communication path between every client and server, a call portal and a return portal. The return portal may be shared,
   - a short-circuit portal implements the operation without switching to another protection domain. It is similar to the Unix null system call getpid(),
   - managing the portal table is crucial for enforcing protection (file access, server access, etc.) and safety (sandboxing, interposition, etc.),
   - a domain may invoke only the portals that it can explicitly open (domain which exports the service, controls who can open portals to it, either directly or via help of security server) or those that it inherited from its parent.
   - each protection domain has a private portal table.
   - portal code and portal tables may be shared between domains.

#### Dynamic Code Generation

 * The portal code is specialized for the particular client and server.
 * The portal code is instantiated from a template at the time that the portal is opened.
 * Portal manager generates portal code from a simple IDL. The portal specification includes:
   * amount of state to save/restore (which registers to save),
   * level of trust between invoking and invoked domain (e.g. whether to share stack),
   * types of arguments and their required handling:
     - none
     - a constant
     - a function of nucleus-visible state
     - a piece of code that was registered by a trusted server (how to transform/map arguments: e.g. dynamic sharing of memory windows)
   * identity and entry point of invoked domain.
 * Certain services may generate a set of portals to implemented various functions (e.g. read/write/seek portals for each FD).
 * Pebble replaces generic paths through the kernel with specialized portals. Specialized portals are shorter. This is the reason why Pebble is more efficient than traditional OS on the same hardware.
 * Portals allow the client to call the desired server directly, without kernel de-multiplexing. For example, file operations call the
corresponding server (file system, protocol stack, device driver, etc.) directly.
 * This is the usual time vs. space trade-off: more memory for fast, specialized portals.
 * Portals can replace other kernel data structures, such as the file descriptor table. The portals corresponding with the file operations encapsulate file information.


VM can register with portal manager several code templates, which are executed for particular argument types. E.g. a code requiring mapping
of a page into target PD will need to access VM internal structures, but PM should not know about structure of this access.

Portal registration with portal_manager:
 * server registers a new entry point by specifying portal name, entrypoint address and interface definition,
 * later, a client requests a portal by (fully-qualified) name from trader/portal_manager, which generates the portal code
   and installs it in client's portal table. portal index is returned to the client.
 * to invoke a portal thread then loads portal number into a registers and performs ***a trap instruction***, which transfers control to the nucleus.
 * the nucleus
   - determines which portal shall be executed,
   - looks up the portal code adress and
   - jumps to the portal code
 * the rest of control transfer is performed by portal code (registers save, argument copy, stack change, page mapping).


#### Servers

System is composed out of components.
Components run in their own Protection Domains (which consist of memory pages and portal table which contains addresses of the portal code for portals this component has access to). Both memory pages and portals can be shared with other Protection Domains.
 - Parent PD may share its portal table with its child. Changes to portal table reflected in both parent and child.
 - Child may receive a copy of PT during creation.

* System services are provided by **server components**.
* Servers run in separate protection domains in user mode.
* Unlike applications, servers may be granted limited privileges:
  - access to a portion of the physical memory
  - modify the portal table
  - run with interrupts disabled (only the interrupt dispatcher and scheduler)
  - the scheduler runs with interrupts disabled (Pebble design flaw, scheduler can be implemented lock-free),
  - device drivers have device registers mapped into their memory region,
  - and the portal manager may add portals to protection domains (a protection domain can not modify its portal table directly).
* Some servers are trusted (not malicious) and form the TCB.
* Errors are localized in the faulty server.


#### Scheduling and Synchronization

* The scheduler is a replaceable component.
* The scheduler runs in user mode.
* The scheduler implements all actions that may change the calling thread’s state (e.g. run → blocked and blocked → ready).
* Threads cannot block anywhere except inside the scheduler. The scheduler knows all waiting and active threads.
* The scheduler provides synchronization via semaphores.
  - Semaphores are implemented by a pair of portals (for the P and V operations)


#### Interrupt Processing

* Interrupts are delivered promptly, since most of the system executes in user-mode with interrupts enabled.
* The scheduler unifies the handling of interrupts and threads. Interrupt handler threads are scheduled according to priority.
* Interrupt latency is determined mostly by the length of the longest interrupt-disabled path (in a portal and in the scheduler).
  - The only parts of Pebble that run with interrupts disabled are portals, the interrupt dispatcher and the scheduler (a design trade-off; will run with interrupts enabled in a future release).
  - Most Pebble servers and all user applications execute in user-mode with interrupts enabled, interrupts are not delayed.
  - Thus: interrupt latency is lower
* Portals implement different functions:
  - portal1 saves the entire state and switches to the interrupt stack.
  - portal2 saves few registers and shares stack with scheduler.
  - portal3 restores entire state and switches to user stack (runs the thread).
* When interrupt comes, the interrupt portal switches to thread's interrupt stack, saves state on invocation stack and passes control to interrupt dispatcher.
* The scheduler handles priority inversion.
* The scheduler can either decide to return to the thread that was interrupted, or it can queue that thread (preempt it), select another
thread and return to it.
* Due to the nature of interrupts, their portals must always be available. Thus interrupt portals belong to a special class of global portals which are the same in all domains. Exceptions which are related to the execution of the processor (e.g. floating-point-underflow), are mapped into portals that are specific to each domain.

#### Related Work

Pebble includes a combination of old ideas:
* Micro-kernels: Mach and L4.
* Protection domains and portals: SPACE.
* Specialized code generation: Synthesis and Synthetix.
* Efficient parameter passing: LRPC.
* Object oriented OS: Spring.
* Dynamically replaceable components: Kea.
* User-level implementation of high-level abstractions: Exokernel

### SPACE

There are two primary operations supported by the SPACE kernel: `portal_entry` which is used to move between domains, and `resume_pcb` which resumes execution at a point where a portal entry had occurred. `portal_entry` can be implicit (e.g. a pagefault or i/o interrupt), or explicit through a kernel call. `resume_pcb` is always an explicit operation.

When a processor switches domains by passing through a portal, the code sequence that was previously running usually needs to be resumed at some time. This requires that the processor state be saved. The SPACE kernel saves the processor state, as well as the identity of the current domain, in a data structure called the processor control block (pcb). A new pcb may be allocated for processor execution at every portal entry, but it is only used if the code sequence in the new domain also enters a portal. Each pcb has a unique token associated with it. The new domain is recorded in the pcb as the owner of the token, so that only code sequences running in the new domain can use the token to do a resume_pcb.

### FLUKE

*State exportability is not yet defined in Vesper.*

Easily exportable kernel state for process to support user-level checkpointing etc.
Pickling - saving exportable state.

Flobs encapsulate all user-visible kernel state in well deﬁned objects. For example, a thread encapsulates the ﬂow
of control in an address space. A mapping defines a range of memory imported to a space, while a region defines a
range of memory that can be exported to other spaces. A mutex provides one synchronization primitive.
Fluke represents pointers to flobs with references, so a mapping contains a reference to the region.

State can be retrieved or set at any given point in time, with as little overhead as possible (hence userspace part of flobs in Fluke).

Providing consistent kernel object state at arbitrary times requires a fundamental property of all kernel operations:
every kernel operation must be either *transparently atomic or restartable*. If they were not, an object could have state stored in internal kernel data structures that is not exported by normal kernel operations. The Fluke kernel provides this transparent interruption and restart.

In Fluke, all requests for services initially go to the parent process. As the parent process, a checkpointer can know what references were granted to the child, and more importantly, it can know what they *logically* point to. Understanding the logical connection is what allows the checkpointer to correctly re-establish external connections upon restart.
Thus, Nested Process Model is crucial in supporting transparent resource virtualization/management by parent processes.

#### Full Fluke API listing

Thread
```
fluke_thread_create
fluke_thread_create_hash
fluke_thread_destroy (cannot destroy itself)
fluke_thread_disable_exceptions (can only disable for itself)
fluke_thread_enable_exceptions (can only enable for itself)
fluke_thread_get_client (preliminary iface)[itself]
fluke_thread_get_handlers [itself]
fluke_thread_get_saved_state [itself]
fluke_thread_get_server (preliminary iface)[itself]
fluke_thread_get_state (cannot call on itself)
fluke_thread_interrupt (cannot call on itself)
fluke_thread_move (cannot call on itself)
fluke_thread_reference
fluke_thread_return_from_exception [itself]
fluke_thread_self [itself]
fluke_thread_set_client (preliminary iface)[itself]
fluke_thread_set_handlers [itself]
fluke_thread_set_saved_state [itself]
fluke_thread_set_server (preliminary iface)[itself]
fluke_thread_set_state (cannot call on itself)
fluke_thread_schedule  (cannot call on itself) // donate cpu to another thread
```

Task
```
fluke_task_create
fluke_task_create_hash
fluke_task_destroy
fluke_task_get_state
fluke_task_move
fluke_task_reference
fluke_task_set_state
```

Region
```
fluke_region_create
fluke_region_create_hash
fluke_region_destroy
fluke_region_get_state
fluke_region_move
fluke_region_protect
fluke_region_reference
fluke_region_search (can search in other tasks)
fluke_region_set_state
```

Mapping
```
fluke_mapping_create
fluke_mapping_create_hash
fluke_mapping_destroy
fluke_mapping_get_state
fluke_mapping_move
fluke_mapping_protect
fluke_mapping_reference
fluke_mapping_set_state
```

Port // Fluke IPC endpoints
```
fluke_port_create
fluke_port_create_hash
fluke_port_destroy
fluke_port_get_state
fluke_port_move
fluke_port_reference
fluke_port_set_state
```

Port set
```
fluke_pset_create
fluke_pset_create_hash
fluke_pset_destroy
fluke_pset_get_state
fluke_pset_move
fluke_pset_reference
fluke_pset_set_state
```

IPC // C interface, not directly mapped to kernel API
```
fluke_ipc_call // syncronous idempotent call
fluke_ipc_client_connect_send // create a reliable connection to a server
fluke_ipc_client_connect_send_over_receive // perform a reliable IPC to a server (client_{connect_send+over_receive})
fluke_ipc_reply // reply to an idempotent call
fluke_ipc_reply_wait_receive // reply to an idempotent call and wait for a new request (ipc_{reply+wait_receive})
fluke_ipc_send // send a one-way message to a port
fluke_ipc_server_ack_send_wait_receive // reply to a reliable RPC and wait for another (ipc_{server_ack_send+wait_receive})
fluke_ipc_server_send_wait_receive // send data to a reliable RPC connection, disconnect and wait for a new invocation (ipc_{server_send+wait_receive})
fluke_ipc_setup_wait_receive // set up a server thread and wait for incoming IPC invocations
fluke_ipc_client_ack_send // become the sender on a reliable IPC connection
fluke_ipc_server_ack_send
fluke_ipc_client_ack_send_over_receive // reverse a reliable IPC connection, send a message and reverse again
fluke_ipc_server_ack_send_over_receive
fluke_ipc_client_alert // send an interrupt on a reliable IPC connection
fluke_ipc_server_alert
fluke_ipc_client_disconnect // destroy a reliable IPC connection
fluke_ipc_server_disconnect
fluke_ipc_client_over_receive // reverse the transfer direction of a reliable IPC connection
fluke_ipc_server_over_receive
fluke_ipc_client_receive // receive data through reliable IPC
fluke_ipc_server_receive
fluke_ipc_client_send // send data across a reliable IPC connection
fluke_ipc_server_send
fluke_ipc_client_send_over_receive // send a message on a reliable IPC connection and reverse the connection
fluke_ipc_server_send_over_receive
fluke_ipc_wait_receive // wait on a port set for incoming IPC invocations
```

Mutex
```
fluke_mutex_create
fluke_mutex_create_hash
fluke_mutex_destroy
fluke_mutex_get_state
fluke_mutex_lock
fluke_mutex_move
fluke_mutex_reference
fluke_mutex_set_state
fluke_mutex_trylock
fluke_mutex_unlock
```

Condition
```
fluke_cond_broadcast
fluke_cond_create
fluke_cond_create_hash
fluke_cond_destroy
fluke_cond_get_state
fluke_cond_move
fluke_cond_reference
fluke_cond_set_state
fluke_cond_signal
fluke_cond_wait
```

Reference
```
fluke_ref_check
fluke_ref_compare
fluke_ref_copy
fluke_ref_create
fluke_ref_destroy
fluke_ref_hash
fluke_ref_move
fluke_ref_type
```


### MISC

#### Continuations vs threads

Any continuation to which a user program can generate a valid reference is already suspended and waiting for a message.
`Throw()` and `ThrowCC()` just hand the suspended continuation a message and run it. `ThrowCC()` suspends the current
continuation and passes it as part of the message.

```
throw_cc(scheduler,&message_to_scheduler,sizeof(message_to_scheduler),&received_message,sizeof(received_message));
```

For `scheduler` being some kind of identifier naming the suspended continuation of a scheduler (could be anything,
but a scheduler in this case), `message_to_scheduler` being (of course) a message to the scheduler, and `received_message` being a buffer to receive a message in when another continuation throws to this one.

Since `ThrowCC()` was invoked instead of `Throw()`, the kernel transforms the running CC into a suspended continuation
and prepends the identifier to that continuation to the intended message, creating the message it finally passes
to the invoked scheduler continuation. In the case of plain `Throw()`, the kernel would just pass the message along
directly as it switches control of the processor, destroying the CC that called `Throw()`.

> [...] A continuation is what remains of an unfinished computation. [...]

#### Memory Barriers (from GCC docs)

**an acquire barrier**. This means that references after the builtin cannot move to (or be speculated to) before the builtin, but previous memory stores may not be globally visible yet, and previous memory loads may not yet be satisfied.

**a release barrier**. This means that all previous memory stores are globally visible, and all previous memory
loads have been satisfied, but following memory reads are not prevented from being speculated to before the barrier.


## VESPER

**nucleus_t**
  - domain traversal supported.

**domain_t** // (@sa Pebble's domain_fork)
  - contains a portal_table with indexes into available portals as means of crossing domain boundaries

**scheduler_t (server)**
  - thread management,
  - blocks and switches threads,
  - not really privileged, other than having interrupts disabled during execution in some implementations,
  - threads block here.
  - theoretically, thread abstraction can also be removed from nucleus into the scheduler_t server (TODO?).
    - what's needed?
      * create a thread in current space_t
      * change thread's runnable property

      `client -> scheduler(create_thread) -> security_server(can_create_thread) -> new thread`

**interrupt_dispatcher_t (server)**
  - abstracts interrupt handling on particular architecture by translating interrupts to portals.

**thread_t** // thread abstraction inside scheduler_t
```cpp
    thread_t
    {
        portal_table_t* ptab; // in domain_t actually
        int32_t* user_stack;
        int32_t* interrupt_stack;
        int32_t* invocation_stack;
    };
```
    @FIXME: how to create a thread that inherits portal table but runs in further subspaces.

**portal_t** // a portal abstraction used inside the kernel to generate, optimize and address portals

**portal_manager_t (server)**
  - space traversal routines (portals) generation,
  - privileged server, has access to spaces in order to add/remove portals to portal_table.

**vm_server_t (server)**
  - serves paged memory to apps,
  - privileged server, has access to all physical memory on the machine (like sigma0 in L4),
  - can map/grant/unmap memory pages upon request (via regions and mappings).

**trader_t (server)**
  - allocates portal IDs for clients
  - lets clients find portal IDs by interface specification
    * adopt sort of dbus-like portal spec with service,interface,method triplet.
  - 0-65535 are system wide portal IDs (available in all PDs).
  - 65536 and above are per-process portal IDs.

**security_server_t (server)**
  - controls who has access where.
  - security_id_t should be adopted in the kernel as security context id to allow simple flask-like object labeling with
    security information.

