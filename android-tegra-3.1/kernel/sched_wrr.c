/*
 * sched_wrr.c
 *
 *  Created on: Oct 28, 2012
 *      Author: w4118
 */

/*
 * Structure of file based on sched_idletask.c
 * implmementation of an idle task scheduler.
 *
 * Methods not currently implemented with functionality.
 *
 * Note: This first will attempt to add schedule for case of 1-CPU, and then
 * later on add support for SMP. This will require additional definition
 * and  implementation SMP methods declared in sched_class in sched.h
 */

/*
 * Main TODO:
 * 1) Make this the default scheduling policy for init and all of its descendants.
 *    Note, May be easiest to have swapper policy be changed, so that kthread
 *    uses WRR as well. This will help ensure system responsiveness.
 * 2) Support Multiprocessor systems, like the Nexus 7.
 * 3) If the weight of a task currently on a CPU is changed, it should finish
 *    its time quantum as it was before the weight change. i.e. increasing
 *    the weight of a task currently on a CPU does not extend its current time
 *    quantum.
 * 4) When deciding which CPU a job should be assigned to, it should be
 *    assigned to the CPU with the smallest total weight (i.e. sum of the
 *    weights of the jobs on the CPU's run queue).
 * 5) Add periodic load balancing every 500ms
 * 6) Only tasks whose policy is set to SCHED_WRR should be considered for
 *    selection by your this scheduler
 * 7) The weight of a task and the SCHED_WRR scheduling flag should be
 *    inherited by the child of any forking task.
 * 8) If a process' scheduler is set to SCHED_WRR after previously being set to
 *    another scheduler, its weight should be the default weight.
 */




#ifdef CONFIG_SMP
static int
select_task_rq_wrr(struct task_struct *p, int sd_flag, int flags)
{
	return task_cpu(p); /* wrr tasks as never migrated */
}
#endif /* CONFIG_SMP */

/* Initializes the given task which is meant to be handled/processed
 * by this scheduler */
static void init_task_wrr(struct task_struct *p)
{
	struct sched_wrr_entity *wrr_entity;
	if (p == NULL)
		return;

	wrr_entity = &p->wrr;
	wrr_entity->task = p;
	wrr_entity->weight = SCHED_WRR_DEFAULT_WEIGHT;
	wrr_entity->time_slice =
			SCHED_WRR_DEFAULT_WEIGHT * SCHED_WRR_TIME_QUANTUM;
	wrr_entity->time_left = wrr_entity->time_slice / SCHED_WRR_TICK_FACTOR;
}


/*
 * This function checks if a task that entered the runnable state should
   preempt the currently running task.
 */
static void check_preempt_curr_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	/* TO be Implemented. */

	/* Idle task impl:
	 * Idle tasks are unconditionally rescheduled:
	 * resched_task(rq->wrr);
	 */

}

/* This function chooses the most appropriate task eligible to run next.*/
static struct task_struct *pick_next_task_wrr(struct rq *rq)
{
	/* TO be Implemented. */
	/*
	if (printk_ratelimit(  ))
		printk("We were called to schedule a task but we have not"
			"implemnetedthe schedule yet!\n");
	*/
	return NULL;

	/** IDLE Task Impl
	 * schedstat_inc(rq, sched_gowrr);
	 * calc_load_account_wrr(rq);
	 */
}

/*
 * When a task is no longer runnable, this function is called.
 * We should decrements the nr_running variable in wrr_rq struct.
 */
static void
dequeue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	return ;
	/* Idle task iMPLM
	raw_spin_unlock_irq(&rq->lock);
	printk(KERN_ERR "bad: scheduling from the wrr thread!\n");
	dump_stack();
	raw_spin_lock_irq(&rq->lock);
	*/
}

/*
 * Called when a task enters a runnable state.
 * It puts the scheduling entity (task) into the red-black tree and
 * increments the nr_running variable.
 */
static void
enqueue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	list_head *head;
	struct sched_wrr_entity *new_entity;
	struct sched_wrr_entity *wrr_entity;
	struct wrr_rq *wrr_rq = &rq->wrr;

	/* spin_lock(wrr_rq->wrr_rq_lock); */

	wrr_entity = &wrr_rq->run_queue;

	init_task_wrr(p); /* initializes the wrr_entity in task_struct */
	new_entity = &p->wrr;

	/* add it to the queue.*/
	head = &wrr_entity->run_list;
	list_add_tail(&new_entity->run_list, head);

	/* update statistics counts */
	++wrr_rq->nr_running;
	++wrr_rq->size;

	/*
	 * TODO: Check and see if we need to use locks here.
	 */


	/* spin_unlock(wrr_rq->wrr_rq_lock); */
}

/* This function is basically just a dequeue followed by an enqueue, unless the
   compat_yield sysctl is turned on */
static void yield_task_wrr (struct rq *rq)
{
	/* To be implemented */
}

/* This method should put the task back of the run queue.
 * For example, it's called in set_scheduler system call that changes
 * the scheduling policy for a task/process. (sched.c #5244)
 *
 * */
static void put_prev_task_wrr(struct rq *rq, struct task_struct *prev)
{
	/* To be implemented */
}

/* This function is mostly called from time tick functions; it might lead to
   process switch.  This drives the running preemption.*/
static void task_tick_wrr(struct rq *rq, struct task_struct *curr, int queued)
{
	/* To be implemented */
	struct sched_wrr_entity *wrr_entity = curr->wrr;
	struct wrr_rq *wrr_rq = &rq->wrr;

	/*TODO: Continue from here ... Add timing statistics
	 * i.e. reduce current time slice value.
	 * sched_fair #558
	 * sched_rt #660
	 *
	 * each tick is worth 10 milliseconds.
	 * */
	if(--wrr_entity->time_left) /* there is still time left */
		return;

	wrr_entity->time_left = wrr_entity->time_slice / SCHED_WRR_TICK_FACTOR;
}

/* This function is called when a currently running task changes its
 * scheduling class or changes its task group. For example, setscheduler
 * calls this function, when a scheduling policy change is requested.
 */
static void set_curr_task_wrr(struct rq *rq)
{
	/* we need to do update our 'curr' pointer.
	 * so that we point to the running task.
	 * rq->curr WILL point to the current running task on the rq.
	 * it's updated in the __schedule() function
	 */
	struct task_struct *p = rq->curr;

	rq->wrr->curr = p;
}

/* This function is called when a running process has changed its scheduler
 * and chosen to make this scheduler (WRR), its scheduler.
 * */
static void switched_to_wrr(struct rq *rq, struct task_struct *p)
{
	/* Still to be tested */

	/* There are two cases here:
	 * 1) switched_to called when current process changed the
	 * policy of *another* non-running process to be SCHED_WRR.
	 * enqueue WILL ALREADY have been called (sched.c #5259)
	 * This will be an extremely unlikely condition, when we make SCHED_WRR
	 * default system scheduler, since all processes will be SCHED_WRR
	 *
	 * 2) The current running process has decided to change its
	 * scheduler to SCHED_WRR from something else, so enqueue it.
	 * set_curr_task will have been called. But enqueue will not have been.
	 */

	if (rq->curr == p) { /* Case 2  */
		printk("switch to wrr: Case 2 Happened");
		enqueue_task_wrr(rq, p, NULL);
	} else
		printk("switch to wrr: Case 2 didn't happen");
}

static void
prio_changed_wrr(struct rq *rq, struct task_struct *p, int oldprio)
{
	/* DO NOT need to implement. Scheduler has no
	 * notion of priority.*/
}

static unsigned int get_rr_interval_wrr(struct rq *rq, struct task_struct *task)
{
	/* To be implemented */
	return 0;
}

/* Set the SCHED_WRR weight of process, as identified by 'pid'.
 * If 'pid' is 0, set the weight for the calling process.
 * System call number 376.
 */
SYSCALL_DEFINE2(sched_setweight, pid_t, pid, int, weight)
{
	/* To be implemneted */
	return -1;
}


/* Obtain the SCHED_WRR weight of a process as identified by 'pid'.
 * If 'pid' is 0, return the weight of the calling process.
 * System call number 377.*/
SYSCALL_DEFINE1(sched_getweight, pid_t, pid)
{
	/*To be implemented */
	return -1;
}


/*
 * Simple, special scheduling class for the per-CPU wrr tasks:
 */
static const struct sched_class wrr_sched_class = {
	/* .next is Fair Scheduler class scheduler */
	.next			= &fair_sched_class,

	/* no enqueue/yield_task for idle tasks */
	/* TODO: Add enqueue/yield task for WRR */

	.enqueue_task 		= enqueue_task_wrr,

	.yield_task		= yield_task_wrr,

	.dequeue_task		= dequeue_task_wrr,

	.check_preempt_curr	= check_preempt_curr_wrr,

	.pick_next_task		= pick_next_task_wrr,
	.put_prev_task		= put_prev_task_wrr,

#ifdef CONFIG_SMP
	/*
	 * TODO:
	 * Add more methods here when we're working on adding
	 * SMP support. sched_rt.c Should give a good example
	 * to follow from.
	 */
	.select_task_rq		= select_task_rq_wrr,
#endif

	.set_curr_task          = set_curr_task_wrr,
	.task_tick		= task_tick_wrr,

	.get_rr_interval	= get_rr_interval_wrr,

	.prio_changed		= prio_changed_wrr,
	.switched_to		= switched_to_wrr,
};
