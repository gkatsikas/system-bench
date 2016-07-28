# system-bench
A set of benchmarks to measure critical system operations such as context switching time and the impact of using different scheduling policies on the context switching.

To measure context switching with different schedulers, change to folder **context-switching** and do:
  * make
  * ./cont_sw_exe -c <core> -s <sched_policy> -p <sched_priority>
Example usage: ./cont_sw_exe -c <core> [-s <sched_policy> -p <sched_priority>]
|->   sched_policy: normal,batch,rr,fifo
|-> sched_priority: [-19,19] for normal,batch or [1,99] for rr,fifo
