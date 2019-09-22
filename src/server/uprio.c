# include "uprio.h"

int setPrio (int which, int who, int prioValue)
{
	int ret = syscall(SYS_ioprio_set, which, who, prioValue);
	return ret;
}
int getPrio (int which, int who)
{
	int prioValue = syscall(SYS_ioprio_get, which, who);
	return prioValue;
}
int getPrioValue (int prioClass, int prioData)
{
	return ((prioClass << IOPRIO_CLASS_SHIFT) | prioData);
}
int getPrioClass (int prioValue)
{
	return (prioValue >> IOPRIO_CLASS_SHIFT);
}
int getPrioData (int prioValue)
{
	return (prioValue & IOPRIO_PRIO_MASK);
}
char * prioClassLookup (int prioClass)
{
	switch (prioClass) {
		case IOPRIO_CLASS_NONE:
			return "IOPRIO_CLASS_NONE";
		case IOPRIO_CLASS_RT:
			return "IOPRIO_CLASS_RT";
		case IOPRIO_CLASS_BE:
			return "IOPRIO_CLASS_BE";
		case IOPRIO_CLASS_IDLE:
			return "IOPRIO_CLASS_IDLE";
	}
	return NULL;
}
