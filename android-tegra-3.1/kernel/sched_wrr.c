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
 *
 *  Other Useful files
 *  ====================
 *  -> init/main.c : Bootup process code. #358
 *  -> include/linux/init_task.h :  Defines the INIT "swapper" task
 *  -> arch/arm/kernel/process.c : contains the kernel_thread() function
 *
 */


/* Forward declaration. Definition found at bottom of this file */
static const struct sched_class wrr_sched_class;

#ifdef CONFIG_SMP
static int
select_task_rq_wrr(struct task_struct *p, int sd_flag, int flags)
{
	return task_cpu(p); /* wrr tasks as never migrated */
}
#endif /* CONFIG_SMP */

/* Returns the size of the list, assuming that it has
 * an explicit head element. */
static int list_size(struct list_head *head)
{
	int count = 0;
	struct list_head *curr;
	for (curr = head->next; curr != head; curr = curr->next) {
		++count;
	}
	return count;
}

/* Debugging helper method */
static void print_wrr_task(struct task_struct *p)
{
	struct sched_wrr_entity *wrr_entity;
	if (p == NULL)
		return;

	wrr_entity = &p->wrr;
	printk("Task Pid: %d\n", p->pid);
	printk("Task Name: %s\n", p->comm);
	printk("WRR Weight: %d\n", wrr_entity->weight);
	printk("WRR Time_Left: %ld\n", wrr_entity->time_left);
	printk("WRR Time Slice:%ld\n", wrr_entity->time_slice);

}

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

/* Return a pointer to the embedded sched_wrr_entity */
static struct sched_wrr_entity *sched_wrr_entity_of_task(struct task_struct *p)
{
	if (p == NULL)
		return NULL;

	return &p->wrr;
}

/* Return a pointer to the sched_wrr specific run queue */
static struct wrr_rq *sched_wrr_rq(struct rq *rq)
{
	if(rq == NULL)
		return NULL;
	return &rq->wrr;
}

/* Return the task_struct in which the given @wrr_entity is embedded inside */
static struct task_struct *wrr_task_of(struct sched_wrr_entity *wrr_entity)
{
	return container_of(wrr_entity, struct task_struct, wrr);
}

/* Return the wrr_rq (run queue struct) in which the given enitity belongs */
static struct wrr_rq *wrr_rq_of_wrr_entity(struct sched_wrr_entity *wrr_entity)
{
	struct task_struct *p = wrr_task_of(wrr_entity);
	struct rq *rq = task_rq(p);

	return &rq->wrr;
}

/* Helper method that determines if the given entity is already
 * in the run queue. Return 1 if true and 0 if false */
static inline int on_wrr_rq(struct sched_wrr_entity *wrr_entity)
{
	/* If any item is on the wrr_rq, then the run
	 * list will NOT be empty. */
	if (list_empty(&wrr_entity->run_list))
		return 0;
	else
		return 1;
}

static void print_queue(struct sched_wrr_entity *head_entity)
{
	struct list_head *curr;
	struct sched_wrr_entity *curr_entity;
	struct task_struct *p;
	int counter = 1;
	struct list_head *head = &head_entity->run_list;
	for (curr = head->next; curr != head; curr = curr->next) {
		curr_entity = list_entry(curr, struct sched_wrr_entity,
					 run_list);

		p  = container_of(curr_entity, struct task_struct, wrr);


		printk("Queue Item %d\n", counter);
		printk("--------------------\n");
		printk("WRR Weight: %d\n", curr_entity->weight);
		printk("Process PID:%d\n", p->pid);
		printk("Process Name: %s\n", p->comm);
		printk("Process TGID:%d\n", p->tgid);
		printk("--------------------\n");
		++counter;
	}
}

static void print_front_item(struct sched_wrr_entity *head)
{
	struct task_struct *p;
	struct list_head *first_item = head->run_list.next;
	struct sched_wrr_entity *first =
		list_entry(first_item, struct sched_wrr_entity, run_list);
	p  = container_of(first, struct task_struct, wrr);
	if (first == head) {
		printk("LIST IS EMPTY\n");
	}

	printk("WRR Weight: %d", first->weight);
	printk("Process PID:%d\n", p->pid);
	printk("Process Name: %s", p->comm);
	printk("Process TGID:%d\n", p->tgid);
}

/* Helper method that requeues a task */
static void requeue_task_wrr(struct rq *rq, struct task_struct *p)
{
	struct list_head *head;
	struct sched_wrr_entity *wrr_entity = sched_wrr_entity_of_task(p);
	struct wrr_rq *wrr_rq = sched_wrr_rq(rq);
	head = &wrr_rq->run_queue.run_list;

	/* Check if the task is the only one in the run-queue.
	 * In this case, we won't need to re-queue it can just
	 * leave it as it is. */
	if (wrr_rq->size == 1)
		return;

	/* There is more than 1 task in queue, let's move this
	 * one to the back of the queue */
	list_move_tail(&wrr_entity->run_list, head);
}

/* Update the current runtime statistics. Modeled after
 * update_curr_rt in sched_rt.c  */
static void update_curr_wrr(struct rq *rq)
{
	struct task_struct *curr = rq->curr;
	/* struct sched_wrr_entity *wrr_entity = &curr->wrr;
	 struct wrr_rq *wrr_rq = wrr_rq_of_wrr_entity(wrr_entity); */

	u64 delta_exec;

	if(curr->sched_class != &wrr_sched_class)
		return;

	delta_exec = rq->clock_task - curr->se.exec_start;
	if(unlikely((s64)delta_exec < 0))
		delta_exec = 0;

	schedstat_set(curr->se.statistics.exec_max,
			max(curr->se.statistics.exec_max, delta_exec));

	curr->se.sum_exec_runtime += delta_exec;
	account_group_exec_runtime(curr, delta_exec);

	curr->se.exec_start = rq->clock_task;
	cpuacct_charge(curr, delta_exec);
}

/*
 * This function checks if a task that entered the runnable state should
   preempt the currently running task.
 */
static void check_preempt_curr_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	/* We never do any preemption since we follow a FIFO Round-Robin
	 * policy that has no notion of priorities. */
}

/* This function chooses the most appropriate task eligible to run next.*/
static struct task_struct *pick_next_task_wrr(struct rq *rq)
{
	/* TO be throughly tested. */
	/*
	if (printk_ratelimit(  ))
		printk("We were called to schedule a task but we have not"
			"implemnetedthe schedule yet!\n");
	*/

	struct task_struct *p;
	struct sched_wrr_entity *head_entity;
	struct sched_wrr_entity *next_entity;
	struct list_head *head;
	struct wrr_rq *wrr_rq =  &rq->wrr;

	if( wrr_rq->nr_running <= 0) /* There are no runnable tasks */
		return NULL;

	/* Pick the first element in the queue.
	 * The item will automatically be re-queued back, in task_tick
	 * funciton */
	head_entity = &wrr_rq->run_queue;
	head = &head_entity->run_list;

	next_entity = list_entry(head->next, struct sched_wrr_entity, run_list);

	p = wrr_task_of(next_entity);
	p->se.exec_start = rq->clock_task;

	return p;

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
	/*
	 * TODO: Check to see if we need
	 * to have any locks here.
	 */

	struct sched_wrr_entity *wrr_entity = &p->wrr;
	struct wrr_rq *wrr_rq = wrr_rq_of_wrr_entity(wrr_entity);

	update_curr_wrr(rq);
	if (!on_wrr_rq(wrr_entity)) { /* Should not happen */
		printk("Invalid Dequeue task for Process '%s' (%d)\n",
			p->comm, p->pid);
		BUG();
		dump_stack();
	}

	/* Remove the task from the queue */
	list_del(&wrr_entity->run_list);

	/* update statistics counts */
	--wrr_rq->nr_running;
	--wrr_rq->size;

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

	struct list_head *head;
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

/* This function is called when a task voluntarily gives up running */
static void yield_task_wrr (struct rq *rq)
{
	/* So we just requeue the task */
	requeue_task_wrr(rq, rq->curr);
}


static void put_prev_task_wrr(struct rq *rq, struct task_struct *prev)
{

	/* From testing, I have found this method is called
	 * when a task is about to exit.
	 * -> function cgroup_exit() in exit.c # 997
	 * and in the main schedule function.
	 * -> function schedule()  sched.c # 4335
	 */

	/* This method is really just a notification to us before
	 * potentially *another* task that we don't manage/schedule
	 * runs and before a task fully exits.
	 *
	 * As such, we don't need to do anything special.
	 * (in the exiting case, dequeue_task will be called)
	 * */
	update_curr_wrr(rq);
	prev->se.exec_start = 0;
}


/* This function is mostly called from time tick functions; it might lead to
   process switch.  This drives the running preemption.*/
static void task_tick_wrr(struct rq *rq, struct task_struct *curr, int queued)
{
	/* Still to be tested  */
	struct sched_wrr_entity *wrr_entity = &curr->wrr;
	struct timespec now;
	getnstimeofday(&now);

	/* Update the current run time statistics. */
	update_curr_wrr(rq);

	/*
	 * each tick is worth 10 milliseconds.
	 * this is based on way that sched_rt is implemented.
	 * (Not exactly sure if this is a hard 10 milliseconds...)

	 * TODO: confirm exact tick rate
	 * My testing on the emulator has revealed that
	 * task_tick_wrr is indeed called every ~10ms. Don't know about the
	 * actual device though.
	 */
	if (--wrr_entity->time_left) /* there is still time left */
		return;

	/* Code Snippet below Measures how often ticks occur */
	/* printk("Time: second=%ld\nnano_second=%ld\n",
			now.tv_sec, now.tv_nsec); */


	/* the time_slice is in milliseconds and we need to
	 * convert it to ticks units */
	wrr_entity->time_left = wrr_entity->time_slice / SCHED_WRR_TICK_FACTOR;

	/* Requeue to the end of the queue if we are not the only
	 * task on the queue (i.e. if there is more than 1 task) */
	if (wrr_entity->run_list.prev != wrr_entity->run_list.next) {
		requeue_task_wrr(rq, curr);
		/* Set rescheduler for later since this function
		 * is called during a timer interrupt */
		set_tsk_need_resched(curr);
	} else {
		 /* No need for a requeue
		  * TODO:  Check to see if we need a
		  * put a resched call here since the
		  * we need to be running the same task.
		  */
		set_tsk_need_resched(curr);
		if (printk_ratelimit(  ))
			printk("No Need for a requeue !\n");
	}
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

	p->se.exec_start = rq->clock_task;

	rq->wrr.curr = &p->wrr;
}

/* This function is called when a running process has changed its scheduler
 * and chosen to make this scheduler (WRR), its scheduler.
 *
 * It is important to note that switched won't be called if there was no
 * actual change in the scheduler class. (e.g. if only the priority number
 * alone changed)
 * */
static void switched_to_wrr(struct rq *rq, struct task_struct *p)
{

	/* Still to be tested */

	/* From testing, I have found that this method
	 * should not need to do anything.
	 *
	 * This is because enqueue will already have been called
	 * by the time we get called.
	 * */


	/* There are two cases here:
	 * 1) switched_to called when current process changed the
	 * policy of *another* non-running process to be SCHED_WRR.
	 * enqueue WILL ALREADY have been called (sched.c #5259)
	 * This will be an extremely unlikely condition, when we make SCHED_WRR
	 * default system scheduler, since all processes will be SCHED_WRR
	 *
	 * 2) The current running process has decided to change its
	 * scheduler to SCHED_WRR from something else, so enqueue it.
	 * set_curr_task will have been called. Enqueue WILL also
	 * have been called.
	 */

	/* TO DELETE THIS
	if (rq->curr == p) { // Case 2
		struct sched_wrr_entity *wrr_entity =
				sched_wrr_entity_of_task(p);
		printk("switch to wrr: Case 2 Happened\n");
		printk("Before enqueue:\n");
		printk("Queue Size: %ld\n",
				rq->wrr.size);
		printk("Queue Size (LM): %d\n",
				list_size(&rq->wrr.run_queue.run_list));
		if(on_wrr_rq(wrr_entity)) {
			printk("ERROR : Entity found in  before Addition\n" );
		} else {
			printk("Everything OK");
		}
		print_queue(&rq->wrr.run_queue);
		enqueue_task_wrr(rq, p, 0);
		printk("After enqueue\n");
		printk("Queue Size: %d\n",
				list_size(&rq->wrr.run_queue.run_list));
		if(on_wrr_rq(wrr_entity)) {
			printk("Everything OK");
		} else {
			printk("ERROR : Entity found in  before Addition\n" );
		}

	} else // Assume case 1
		printk("switch to wrr: Assuming Case 1\n");
	*/
}

static void
prio_changed_wrr(struct rq *rq, struct task_struct *p, int oldprio)
{
	/* DO NOT need to implement. Scheduler has no
	 * notion of priority.*/
}

/* Would be called by the get_interval system call
 * sched_rr_ get_interval */
static unsigned int get_rr_interval_wrr(struct rq *rq, struct task_struct *task)
{
	/* To be implemented */
	if (task == NULL)
		return -1;
	return task->wrr.weight * SCHED_WRR_TIME_QUANTUM /
			SCHED_WRR_TICK_FACTOR ;
}

/* Set the SCHED_WRR weight of process, as identified by 'pid'.
 * If 'pid' is 0, set the weight for the calling process.
 * System call number 376.
 */
SYSCALL_DEFINE2(sched_setweight, pid_t, pid, int, weight)
{
	/* TODO: To be implemneted */
	return -1;
}


/* Obtain the SCHED_WRR weight of a process as identified by 'pid'.
 * If 'pid' is 0, return the weight of the calling process.
 * System call number 377.*/
SYSCALL_DEFINE1(sched_getweight, pid_t, pid)
{
	/* TODO: Question:  Do we need to hold a lock of
	 * some kind here ? It's possible, though unlikely
	 * that a task dies completely during our call.
	 * Investigate locks for the PID_HASH table structures*/

	/* Note, there is no Need to for user access controls.
	 * All Users all allows to see anyone's weight.
	 *
	 * Further on ARM systems, pid_t is just an int
	 * so, we can access it directly.
	 */

	int result = 0;
	struct task_struct* task = NULL;
	struct pid *pid_struct = NULL;

	if (pid < 0)
		return -EINVAL;

	pid_struct = find_get_pid(pid);

	if (pid_struct == NULL) { /* Invalid PID */
		return -EINVAL;
	}

	/* TODO: Question: Should we only return PID weights
	 * for process only ? */

	task = get_pid_task(pid_struct, PIDTYPE_PID);

	print_wrr_task(task);

	result = task->wrr.weight;

	return result;
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
