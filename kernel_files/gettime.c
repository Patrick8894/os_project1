#include<linux/linkage.h>
#include<linux/kernel.h>
#include<linux/ktime.h>
#include<linux/uaccess.h>

asmlinkage void sys_gettime(struct timespec *ut)
{
	struct timespec t;
	getnstimeofday(&t);
	copy_to_user(ut, &t, sizeof(struct timespec));
	return;
}
