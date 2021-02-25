#ifndef _KERN_PROC_PPROC_H_
#define _KERN_PROC_PPROC_H_

#ifdef _KERN_

void naan_lock_init(void);

void naan_lock(void);

void naan_unlock(void);

void n_produce(void);

void n_consume(void);

unsigned int n_checkfull(void);

unsigned int n_checkempty(void);

void prod_wait (void);

void prod_signal (void);

void cons_wait(void);

void cons_signal(void);

unsigned int n_check(void);

void producer_queue_init(void);
void consumer_queue_init(void);
void naan_lock_init(void);

#endif  /* _KERN_ */

#endif  /* !_KERN_PROC_PPROC_H_ */