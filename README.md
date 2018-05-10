System Benchmarks
=========

A set of benchmarks to measure critical system operations such as context switching time and the impact of using different scheduling policies on the context switching.

Context Switching
----

To measure context switching with different schedulers, change to folder [context-switching][const_sw] and do:
  * `make`
  * `./cont_sw_exe -c <core> -s <sched_policy> -p <sched_priority>`

Example usage:

	./cont_sw_exe -c <core> -s [normal or batch]  -p [-19, 19]

	./cont_sw_exe -c <core> -s [rr or fifo]  -p [1,99]

This repository was used to measure the time a modern server requires for context switching, in the context of network functions virtualization (NFV).
An extensive study in NFV profiling along with experimental results are reported in our [Elsevier journal article][scc-article].

```
@article{katsikas-scc.elsevier17,
	author       = {Katsikas, Georgios P. and Maguire Jr., Gerald Q. and Kosti\'{c}, Dejan},
	title        = {{Profiling and accelerating commodity NFV service chains with SCC}},
	journal      = {Journal of Systems and Software},
	month        = feb,
	year         = {2017},
	volume       = {127C},
	pages        = {12-27},
	numpages     = {16},
	issn         = {0164-1212},
	doi          = {10.1016/j.jss.2017.01.005},
	url          = {https://doi.org/10.1016/j.jss.2017.01.005},
	publisher    = {Elsevier},
	keywords     = {NFV, service chains, profiler, scheduling, I/O multiplexing},
}
```

[const_sw]: context-switching/
[scc-article]: http://www.sciencedirect.com/science/article/pii/S0164121217300055
