#define _GNU_SOURCE
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sched.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<sys/syscall.h>
#include<signal.h>
#include<sys/wait.h>

typedef struct Process{
	char name[30];
	int rtime;
	int etime;
	pid_t pid;
}process;
int scheduler(process *proc, int procnum, int policy);
int main(int argc, char* argv[])
{
	char policyname[128];
	int procnum;
	int policy;
	scanf("%s", policyname);
	scanf("%d", &procnum);
	process *proc = (process *)malloc(procnum * sizeof(process));
	for(int i = 0; i < procnum; i++)
		scanf("%s%d%d", proc[i].name, &proc[i].rtime, &proc[i].etime);
	if(strcmp(policyname, "FIFO") == 0)
		policy = 1;
	else if(strcmp(policyname, "RR") == 0)
		policy = 2;
	else if(strcmp(policyname, "SJF") == 0)
		policy = 3;
	else if(strcmp(policyname, "PSJF") == 0)
		policy = 4;
	else{
		fprintf(stderr, "invalid input");
		exit(0);
	}
	scheduler(proc, procnum, policy);
	exit(0);
}
#define unit_time()				\
{						\
	volatile unsigned long i;		\
	for (i = 0; i < 1000000UL; i++);	\
}						\
	
int cmp(const void *a, const void *b){
	if (((process *)a) -> rtime > ((process*)b) -> rtime);
		return 1;
	return 0;
}
int now, ntime, finished, last_t;
int nextproc(process *proc, int procnum, int policy);
int assign_cpu(int pid, int cpu){
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);
	sched_setaffinity(pid, sizeof(mask), &mask);
	return 0;
}
int procexec(process proc){
	int pid;
	if(pid = fork() == 0){
		unsigned long  start_sec, start_nsec, end_sec, end_nsec;
		syscall(314, &start_sec, &start_nsec);
		for(int i = 0; i < proc.etime; i++)
			unit_time();
		syscall(314, &end_sec, &end_nsec);
		char dmesg[300];
		sprintf(dmesg, "[project1] %d %lu.%09lu %lu.%09lu\n", getpid(), start_sec, start_nsec, end_sec, end_nsec);
		syscall(315, dmesg);
		exit(0);
	}
	assign_cpu(pid, 1);
	return pid;
}
int block_proc(int pid){
	struct sched_param param;
	param.sched_priority = 0;
	int ret = sched_setscheduler(pid, SCHED_IDLE, &param);
	return ret;
}
int p_wakeup(int pid){
	struct sched_param param;
	param.sched_priority = 0;
	int ret = sched_setscheduler(pid, SCHED_OTHER, &param);
	return ret;
}
int nextproc(process *proc, int procnum, int policy){
	if(now != -1 && (policy == 1 || policy == 3))
		return now;
	int choose = -1;
	if(policy == 3 || policy == 4){
		for(int i = 0; i < procnum; i++){
			if(proc[i].pid == -1 || proc[i].etime == 0)
				continue;
			if(choose == -1 || proc[i].etime < proc[choose].etime)
				choose = i;
		}
	}
	else if(policy == 1){
		for(int i = 0; i < procnum; i++){
			if(proc[i].pid == -1 || proc[i].etime == 0)
				continue;
			if(choose == -1 || proc[i].rtime < proc[choose].rtime)
				choose = i;
		}
	}
	else if(policy == 2){
		if(now == -1){
			for(int i = 0; i < procnum; i++){
				if(proc[i].pid != -1 && proc[i].etime > 0){
					choose = i;
					break;
				}
			}
		}
		else if((ntime - last_t) % 500 == 0){
			choose = (now + 1) % procnum;
			while(proc[choose].pid == -1 || proc[choose].etime == 0)
				choose = (choose + 1) % procnum;
		}
		else
			choose = now;
	}
	return choose;
}
int scheduler(process *proc, int procnum, int policy){
	qsort(proc, procnum, sizeof(process), cmp);
	for(int i = 0; i < procnum; i++)
		proc[i].pid = -1;
	assign_cpu(getpid(), 0);
	p_wakeup(getpid());
	ntime = 0, now = -1, finished = 0;
	while(1){
		if(now != -1 && proc[now].etime == 0){
			waitpid(proc[now].pid, NULL, 0);
			printf("%s %d\n", proc[now].name, proc[now].pid);
			now = -1;
			finished++;
			if(finished == procnum)
				break;
		}
		for(int i = 0; i < procnum; i++){
			if(proc[i].rtime == ntime){
				proc[i].pid = procexec(proc[i]);
				block_proc(proc[i].pid);
			}
		}
		int next = nextproc(proc, procnum, policy);
		if(next != -1){
			if(now != next){
				p_wakeup(proc[next].pid);
				block_proc(proc[now].pid);
				last_t = ntime;
				now = next;
			}
		}
		unit_time();
		if(now != -1)
			proc[now].etime--;
		ntime++;
	}
	return 0;
}
