# system-bench
A set of benchmarks to measure critical system operations such as context switching time and the impact of using different scheduling policies on the context switching.

To measure context switching with different schedulers, change to folder [context-switching][const_sw] and do:
  * `make`
  * `./cont_sw_exe -c <core> -s <sched_policy> -p <sched_priority>`

Example usage:

	./cont_sw_exe -c <core> -s [normal or batch]  -p [-19, 19]

	./cont_sw_exe -c <core> -s [rr or fifo]  -p [1,99]

[const_sw]: context-switching/
