#include "import.h"
#include <lib/spinlock.h>
#include <lib/debug.h>
#include <lib/x86.h>
#include <lib/thread.h>

#define MAX_NAAN 7
#define QUEUE_SIZE NUM_IDS

typedef struct Pid_to_cpu {
    unsigned int pid;
    unsigned int cpu_id;
} pid_to_cpu;

pid_to_cpu err;

/* spinlock for naan */
static spinlock_t naan_lk;

void naan_lock_init(void) {
    spinlock_init(&naan_lk);
}

void naan_lock(void) {
    spinlock_acquire(&naan_lk);
}

void naan_unlock(void) {
    spinlock_release(&naan_lk);
}

unsigned int naan = 0;

/* increment */
void n_produce(void) {
    naan++;
}

/* decrement */
void n_consume(void) {
    naan--;
}

/* return 1 if full, else 0 */
unsigned int n_checkfull(void) {
    if (naan >= MAX_NAAN) {
        return 1;
    } 
        
    return 0;
}

/* return 1 if empty, else 0 */
unsigned int n_checkempty(void){
    if(naan <= 0){
        return 1;
    }
    return 0;
}


unsigned int n_check(void) {
    return naan;
}

/* CV queue for Producers */
pid_to_cpu producer_queue[QUEUE_SIZE];
unsigned int prod_front = 0;
unsigned int prod_back = 0;
unsigned int prod_size = 0;

void producer_queue_init(void) {
    for (int i = 0; i < QUEUE_SIZE; i++) {
        producer_queue[i].pid = NUM_IDS;
        producer_queue[i].cpu_id = NUM_CPUS;
    }
    err.pid = NUM_IDS;
    err.cpu_id = NUM_CPUS;
}

unsigned int producer_enqueue(unsigned int id) {
    if (prod_size < QUEUE_SIZE) {
        producer_queue[prod_back].pid = id;
        producer_queue[prod_back].cpu_id = get_pcpu_idx();
        
        prod_back++;
        prod_back = prod_back % QUEUE_SIZE;
        prod_size++;
        //KERN_DEBUG("Added to the queue, proc id: %d, size: %d, prod_back: %d, prod_front %d \n", id, prod_size, prod_back, prod_front);
        return id;
    }

    return NUM_IDS;
}

pid_to_cpu producer_dequeue(void) {
    if (prod_size > 0) {
        pid_to_cpu ret = producer_queue[prod_front];
        producer_queue[prod_front].pid = NUM_IDS;
        producer_queue[prod_front].cpu_id = NUM_CPUS;
        prod_front++;
        prod_front = prod_front % QUEUE_SIZE;
        prod_size--;
        //KERN_DEBUG("Dequeueing from cons queue, proc id: %d, size: %d \n", ret.pid, prod_size);
        return ret;
    }

    return err;
}

/* CV queue for Consumers */
pid_to_cpu consumer_queue[QUEUE_SIZE];
unsigned int cons_front = 0;
unsigned int cons_back = 0;
unsigned int cons_size = 0;

void consumer_queue_init(void) {
    for (int i = 0; i < QUEUE_SIZE; i++) {
        consumer_queue[i].pid = NUM_IDS;
        consumer_queue[i].cpu_id = NUM_CPUS;
    }
}

unsigned int consumer_enqueue(unsigned int id) {
    if (cons_size < QUEUE_SIZE) {
        consumer_queue[cons_back].pid = id;
        consumer_queue[cons_back].cpu_id = get_pcpu_idx();
        cons_back++;
        cons_back = cons_back % QUEUE_SIZE;
        cons_size++;
        //KERN_DEBUG("Added to the cons queue, proc id: %d, size: %d, cons_back: %d, cons_front %d \n", id, cons_size, cons_back, cons_front);
        return id;
    }

    return NUM_IDS;
}

pid_to_cpu consumer_dequeue(void) {
    if (cons_size > 0) {
        pid_to_cpu ret = consumer_queue[cons_front];
        consumer_queue[cons_front].pid = NUM_IDS;
        consumer_queue[cons_front].cpu_id = NUM_CPUS;
        cons_front++;
        cons_front = cons_front % QUEUE_SIZE;
        cons_size--;
        //KERN_DEBUG("Dequeueing from cons queue, proc id: %d, size: %d \n", ret.pid, cons_size);
        return ret;
    }

    return err;
}

void prod_wait(void) {

    thread_lock(NUM_IDS + get_pcpu_idx());

    unsigned int new_cur_pid;
    unsigned int old_cur_pid = get_curid();

    /* check if queue is full */
    if(producer_enqueue(old_cur_pid) == NUM_IDS){
        naan_unlock();
        thread_unlock(NUM_IDS + get_pcpu_idx());
        return;
    }

    /* enqueue current proc that needs to wait to produce */
    tcb_set_state(old_cur_pid, TSTATE_SLEEP);
    new_cur_pid = tqueue_dequeue(NUM_IDS + get_pcpu_idx());
    KERN_ASSERT(new_cur_pid != NUM_IDS);
    tcb_set_state(new_cur_pid, TSTATE_RUN);
    set_curid(new_cur_pid);

    /* CONSIDER: what about the case of no other threads in ready queue for that CPU */
    if (old_cur_pid != new_cur_pid) {
        naan_unlock();
        thread_unlock(NUM_IDS + get_pcpu_idx());
        active_threads_update(get_pcpu_idx(), new_cur_pid); 
        kctx_switch(old_cur_pid, new_cur_pid);
        return;
    }
    naan_unlock();
    thread_unlock(NUM_IDS + get_pcpu_idx());
}

void prod_signal(void) {
    //dequeue from th cv queue -- add to the ready queue 
    pid_to_cpu id = producer_dequeue();
    
    if(id.pid == NUM_IDS){
        return;
    }

    thread_lock(NUM_IDS + id.cpu_id);   
    
    /* enqueue */
    tcb_set_state(id.pid, TSTATE_READY);
    tqueue_enqueue(NUM_IDS + id.cpu_id, id.pid);
    //KERN_DEBUG("Process %d added back to the ready queue for CPU %d\n", id.pid, id.cpu_id);
    thread_unlock(NUM_IDS + id.cpu_id);
}

void cons_wait(void) {
    thread_lock(NUM_IDS + get_pcpu_idx());

    unsigned int new_cur_pid;
    unsigned int old_cur_pid = get_curid();

    /* check if queue is full */
    if(consumer_enqueue(old_cur_pid) == NUM_IDS){
        naan_unlock();
        thread_unlock(NUM_IDS + get_pcpu_idx());
        return;
    }

    /* enqueue current proc that needs to wait to produce */
    tcb_set_state(old_cur_pid, TSTATE_SLEEP);
    new_cur_pid = tqueue_dequeue(NUM_IDS + get_pcpu_idx());
    KERN_ASSERT(new_cur_pid != NUM_IDS);
    tcb_set_state(new_cur_pid, TSTATE_RUN);
    set_curid(new_cur_pid);

    /* CONSIDER: what about the case of no other threads in ready queue for that CPU */
    if (old_cur_pid != new_cur_pid) {
        naan_unlock();
        thread_unlock(NUM_IDS + get_pcpu_idx());
        active_threads_update(get_pcpu_idx(), new_cur_pid); 
        kctx_switch(old_cur_pid, new_cur_pid);
        return;
    }
    naan_unlock();
    thread_unlock(NUM_IDS + get_pcpu_idx());
}

void cons_signal(void) {
    //dequeue from th cv queue -- add to the ready queue 

    pid_to_cpu id = consumer_dequeue();
    if(id.pid == NUM_IDS){
      //  KERN_DEBUG("CONSUMER QUEUE IS EMPTY. \n");
        return;
    }

    thread_lock(NUM_IDS + id.cpu_id);
    
    /* enqueue */
    tcb_set_state(id.pid, TSTATE_READY);
    tqueue_enqueue(NUM_IDS + id.cpu_id, id.pid);
    //KERN_DEBUG("Process %d added back to the ready queue for CPU %d\n", id.pid, id.cpu_id);
    thread_unlock(NUM_IDS + id.cpu_id);
}