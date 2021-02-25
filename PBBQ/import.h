#ifndef _KERN_PROC_PPROC_H_
#define _KERN_PROC_PPROC_H_

#ifdef _KERN_


unsigned int active_threads_check(unsigned int i);

void thread_lock(int thread_lkid);

void thread_unlock(int thread_lkid);

int get_pcpu_idx(void);

void kctx_switch(unsigned int from_pid, unsigned int to_pid);

unsigned int tqueue_dequeue(unsigned int chid);

void tqueue_enqueue(unsigned int chid, unsigned int pid);

unsigned int get_curid(void);

void set_curid(unsigned int curid);

void active_threads_update(unsigned int i, unsigned int new_id);

void tcb_set_state(unsigned int pid, unsigned int state);

#endif  /* _KERN_ */

#endif  /* !_KERN_PROC_PPROC_H_ */