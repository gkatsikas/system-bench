#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <linux/unistd.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#elif MACOS
#include <sys/param.h>
#include <sys/sysctl.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#define MAX_NICE -19
#define MIN_NICE  19

#define WRONG_INPUT_ARGS -1
#define MESSAGES_NB 5000

/*
 * Mapping of scheduling policy IDs (indices) to their names.
 */
const char *SCHED_POLICY_NAMES[] = {
	"SCHED_OTHER",
	"SCHED_FIFO",
	"SCHED_RR",
	"SCHED_BATCH",
};

/*
 * Get the process ID of this process.
 */
pid_t
getpid(void)
{
	return syscall(__NR_getpid);
}

/*
 * Suggest the usgae of this program to the user.
 */
void
usage(const char *program)
{
	fprintf(stderr, "Usage: %s -c <core> [-s <sched_policy> -p <sched_priority>]\n", program);
	fprintf(stderr, "|->   sched_policy: normal,batch,rr,fifo\n");
	fprintf(stderr, "|-> sched_priority: [%d,%d] for normal,batch or [%d,%d] for rr,fifo\n",
			MAX_NICE, MIN_NICE,
			sched_get_priority_min(SCHED_FIFO),  sched_get_priority_max(SCHED_FIFO));
	exit(WRONG_INPUT_ARGS);
}

int get_number_of_cores(void)
{
#ifdef WIN32
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
#elif MACOS
	int nm[2];
	size_t len = 4;
	uint32_t count;

	nm[0] = CTL_HW;
	nm[1] = HW_AVAILCPU;
	sysctl(nm, 2, &count, &len, NULL, 0);

	if (count < 1) {
		nm[1] = HW_NCPU;
		sysctl(nm, 2, &count, &len, NULL, 0);
		if (count < 1) {
			count = 1;
		}
	}
	return count;
#else
	return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

/*
 * Parse the user's input arguments.
 */
void
parse_arguments(
		int   cmd_args_no,
		char  **cmd_args,
		int   *core,
		char  **sched_policy_label,
		short *sched_prio
)
{
	const char *program = cmd_args[0];

	// Check number of arguments
	if ((cmd_args_no != 3) && (cmd_args_no != 5) && (cmd_args_no != 7)) {
		fprintf(stderr, "Wrong arguments\n");
		usage(program);
	}

	// Parse arguments
	while (*cmd_args) {
		// CPU core
		if (strcmp(*cmd_args, "-c") == 0) {
			cmd_args++;
			if (!cmd_args) break;
			*core = atoi(*cmd_args);
		}
		// Scheduling policy
		else if (strcmp(*cmd_args, "-s") == 0) {
			cmd_args++;
			if (!cmd_args) break;
			*sched_policy_label = (char *)*cmd_args;
		}
		// Scheduling priority
		else if (strcmp(*cmd_args, "-p") == 0) {
			cmd_args++;
			if (!cmd_args) break;
			*sched_prio = atoi(*cmd_args);
		}
		cmd_args++;
	}

	*cmd_args = NULL;
}

/*
 * Check if the input arguments are valid.
 */
void
check_arguments(
		const char *program,
		int   core,
		char  **sched_policy_label,
		short *sched_prio,
		short *sched_policy)
{
	int total_cores = get_number_of_cores();

	if ((core < 0) || (core >= total_cores)) {
		fprintf(stderr, "Invalid core number. Correct range is [0, %d]\n", total_cores - 1);
		usage(program);
	}

	// No scheduling policy selected, use default
	if (!(*sched_policy_label)) {
		*sched_policy_label = "normal";
	}

	// Classify the policy
	if (strcmp(*sched_policy_label, "normal") == 0)
		*sched_policy = SCHED_OTHER;
	if (strcmp(*sched_policy_label, "fifo")   == 0)
		*sched_policy = SCHED_FIFO;
	if (strcmp(*sched_policy_label, "rr")     == 0)
		*sched_policy = SCHED_RR;
	if (strcmp(*sched_policy_label, "batch")  == 0)
		*sched_policy = SCHED_BATCH;
	*sched_policy_label = (char *) SCHED_POLICY_NAMES[*sched_policy];

	// Real time policy is selected, check priority
	if ((*sched_policy == SCHED_FIFO) || (*sched_policy == SCHED_RR)) {
		if ((*sched_prio < sched_get_priority_min(SCHED_FIFO)) ||
			(*sched_prio > sched_get_priority_max(SCHED_FIFO))) {
			fprintf(stderr, "Invalid scheduling priority %d for the policy %s\n", *sched_prio, *sched_policy_label);
			usage(program);
		}
	}
	// CFS policy is selected
	else {
		// Negative priorities are used,  hence MAX_NICE has the lowest value, yet maximum priority
		if ((*sched_prio < MAX_NICE) || (*sched_prio > MIN_NICE)) {
			fprintf(stderr, "Invalid scheduling priority %d for the policy %s\n", *sched_prio, *sched_policy_label);
			usage(program);
		}
	}
}

/*
 * Set the process affinity, scheduling policy, and priority
 */
int
schedule(int core, short sched_policy, short sched_prio)
{
	cpu_set_t set;
	struct sched_param prio_param;
	int prio_max;

	CPU_ZERO(&set);
	CPU_SET (core, &set);
	memset(&prio_param, 0, sizeof(struct sched_param));

	// Pin the process to the requested core
	if (sched_setaffinity( getpid(), sizeof( cpu_set_t ), &set )) {
		perror( "Error while setting core affinity using sched_setaffinity()" );
	}

	// Schedule the process according to the user's preference
	if ((sched_policy == SCHED_FIFO) || (sched_policy == SCHED_RR)) {
		prio_param.sched_priority = sched_prio;
	}
	else {
		prio_param.sched_priority = 0;
	}

	// Set the policy and static priority
	if (sched_setscheduler(getpid(), sched_policy, &prio_param) < 0) {
		perror("Error while scheduling using sched_setscheduler()");
	}

	if ((sched_policy == SCHED_FIFO) || (sched_policy == SCHED_RR)) {
		return 0;
	}

	// If CFS, dynamic priority is set via the nice syscall
	int dyn_prio = nice(sched_prio);

	if (dyn_prio != sched_prio)
		perror("Error while setting the dynamic priority using nice()");

	return 0;
}

int main(int argc, char *argv[])
{
	/*********************************************************************************************
	To make sure context-switching processes are located on the same processor:
	1. Bind the processes to a particular processor using sched_setaffinity.
	2. Schedule both processes according to the user-requested policy and priority.
	**********************************************************************************************/
	int   core = -1;
	short sched_prio   = -1;
	short sched_policy = -1;
	char *sched_policy_label = NULL;

	// Read input parameters and do sanity check.
	parse_arguments(argc, argv, &core, &sched_policy_label, &sched_prio);
	check_arguments(argv[0], core, &sched_policy_label, &sched_prio, &sched_policy);

	fprintf(stdout, "Core: %d \n", core);
	fprintf(stdout, "Scheduling   Policy: %s \n", sched_policy_label);
	fprintf(stdout, "Scheduling Priority: %d \n", sched_prio);
	fprintf(stdout, "\n");

	if (schedule(core, sched_policy, sched_prio) != 0) {
		exit(EXIT_FAILURE);
	}

	/*****************************************************************************************************
	Clone the process using fork and share 3 pipes between the parent and children.
	One pipe is used to pass messages from the parent to child and the second for the other way around.
	A third pipe is used to pass timestamps from the child to the parent in order to measure the context
	switch time.
	After the child process to get the current timestamp. This is roughly the difference between two
	timestamps n * 2 times the context switch time.
	*******************************************************************************************************/

	int     i   =  0;
	int     ret = -1;
	int     par_to_child_pipe[2];
	int     child_to_par_pipe[2];
	int     time_pipe[2];

	int     nbytes;
	char    ping_message[] = "Ping!\n";
	char    pong_message[] = "Pong!\n";
	char    child_buffer [20];
	char    parent_buffer[20];
	struct  timeval start,end;

	// Create unnamed pipes
	if (pipe(par_to_child_pipe) == -1)  {
		perror("Error while creating pipe");
	}

	// Create an unnamed second pipe
	if (pipe(child_to_par_pipe) == -1) {
		perror("Error while creating pipe");
	}

	// Create an unnamed time pipe which will share in order to show time spend between processes
	if (pipe(time_pipe) == -1) {
		perror("Error while creating pipe");
	}

	// Clone the process
	if ((ret=fork()) == -1) {
		perror("Error while cloning process with fork");
	}
	// Child
	else if (ret == 0) {
		int n = -1;
		fprintf(stdout, "Child  PID: %6d\n",getpid());

		for (n=0; n<MESSAGES_NB ; n++) {
			nbytes = read(par_to_child_pipe[0], child_buffer, sizeof(child_buffer));
		//	printf("Child  received: %s", child_buffer);
			write(child_to_par_pipe[1], pong_message, strlen(pong_message) + 1);
		}

		gettimeofday(&end, 0);
		n = sizeof(struct timeval);

		if (write(time_pipe[1], &end, sizeof(struct timeval)) != n) {
			perror("Child: Failed to write into the time pipe\n");
		}
	}
	// Parent
	else {
		int n = -1;
		double switch_time;

		fprintf(stdout, "Parent PID: %6d\n", getpid());
		gettimeofday(&start, 0);

		/* Read a string from the pipe */
		for (n=0 ; n<MESSAGES_NB ; n++) {
			write(par_to_child_pipe[1], ping_message, strlen(ping_message) + 1);
			read(child_to_par_pipe[0], parent_buffer, sizeof(parent_buffer));
		//	printf("Parent received: %s", parent_buffer);
		}

		n = sizeof(struct timeval);
		if (read(time_pipe[0], &end, sizeof(struct timeval)) != n) {
			perror("Parent: Failed to read from the time pipe\n");
		}

		wait(NULL);
		switch_time = ((end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec))/1000.0;

		fprintf(stdout, "\n");
		fprintf(stdout, "[Core %2d] Context switch time between two processes: %0.6lf microseconds\n",
				core, (switch_time/(MESSAGES_NB*2))*1000);
	}

	return 0;
}
