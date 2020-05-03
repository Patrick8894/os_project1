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
#include<time.h>

typedef struct Process{
	char name[30];
	int rtime;
	int etime;
	pid_t pid;
}process;
int scheduler(process *proc, int procnum, int policy);
int queue_pos(int pos);
int queue[300], head = 0, tail = 0;
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
//	printf("scan finished\n");
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
	struct timespec tstart, tend;
	syscall(333, &tstart);
	int pid = fork();
	if(pid == 0){
		for(int i = 0; i < proc.etime; i++){
			unit_time();
//			if(i % 100 == 0)
//				printf("process %s %d unit time\n", proc.name, i);
		}
		syscall(333, &tend);
		char dmesg[300];
//		start_sec = tstart.tv_sec;
//		start_nsec = tstart.tv_nsec;
//		end_sec = tend.tv_sec;
//		end_nsec = tend.tv_nsec;
//		sprintf(dmesg, "[project1] %d %lu.%09lu %lu.%09lu\n", getpid(), start_sec, start_nsec, end_sec, end_nsec);
		syscall(334, getpid(), tstart, tend);
		exit(0);
	}
	assign_cpu(pid, 1);
	return pid;
}
int create_mid(){
	int pid = fork();
	if(pid == 0){
		struct sched_param param;
		param.sched_priority = 50;
		int ret = sched_setscheduler(pid, SCHED_FIFO, &param);
		while(1);
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
	param.sched_priority = 99;
	int ret = sched_setscheduler(pid, SCHED_FIFO, &param);
	return ret;
}
int nextproc(process *proc, int procnum, int policy){
	int flag = 0;
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
		for(int i = procnum - 1; i >= 0; i--){
			if(proc[i].pid != -1 && proc[i].etime > 0){
				flag++;
				if(flag > 1)
				break;
			}
		}
//		printf("%d %d\n", ntime, last_t);
//		printf("head = %d\n", head);
		if(now == -1){
			if(flag == 0)
				return -1;
			choose = queue[head];
//			printf("[%d] = %d\n", head, queue[head]);
			head = (head + 1) % 300;
		}
		else if((ntime - last_t) % 500 == 0){
			if(flag == 0)
				return -1;
			if(flag == 1)
				return now;
			choose = queue[head];
			head = (head + 1) % 300;
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
	int mid_pid = create_mid();
	p_wakeup(getpid());
	ntime = 0, now = -1, finished = 0;
//	for(int i = 0; i < 6; i++)
//		printf("proc %d %d %d\n", i, proc[i].rtime, proc[i].etime);
	while(1){
//		printf("head = %d, tail = %d\n", head, tail);
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
//				printf("block %d\n", i);
				queue[tail] = i;
				tail = (tail + 1) % 300;
//				printf("yes");
			}
		}
//		if(ntime % 500 == 0)
//		for(int i = 0; i < 5; i++)
//			printf("queue[%d] = %d\n", i, queue[i]);
		int next = nextproc(proc, procnum, policy);
//		int next;
//		if(ntime % 500 == 0)
//		printf("next = %d, now = %d\n", next, now);
		if(next != -1){
			if(now != next){
				p_wakeup(proc[next].pid);
//				printf("wake up %d %d\n", next, head);
				block_proc(proc[now].pid);
//				printf("block %d\n", now);
				if(now != -1){
					queue[tail] = now;
					tail = (tail + 1) % 300;
				}
				last_t = ntime;
				now = next;
			}
		}
		unit_time();
		if(now != -1)
			proc[now].etime--;
		ntime++;
	}
//	for(int i = 0; i < 300; i++){
//		printf("%d ", queue[i]);
//		if(i % 10 == 0)
//			printf("\n");
//	}
	kill(mid_pid, SIGKILL);
	return 0;
}
