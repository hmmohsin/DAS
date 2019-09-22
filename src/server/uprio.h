# define _GNU_SOURCE
# include <unistd.h>
# include <sys/syscall.h>
# include <sys/types.h>
# include <signal.h>



#define IOPRIO_CLASS_SHIFT      (13)
#define IOPRIO_PRIO_MASK        ((1UL << IOPRIO_CLASS_SHIFT) - 1)

#define IOPRIO_WHO_PROCESS (1)
#define IOPRIO_WHO_PGRP (2)
#define IOPRIO_WHO_USER (3)


#define IOPRIO_CLASS_NONE 0
#define IOPRIO_CLASS_RT 1
#define IOPRIO_CLASS_BE 2
#define IOPRIO_CLASS_IDLE 3

int getPrio (int which, int who);
int setPrio (int which, int who, int prio);
int getPrioValue (int prioClass, int prioData);
int getPrioClass (int prioValue);
int getPrioData (int prioValue);
char * prioClassLookup(int prioClass);
