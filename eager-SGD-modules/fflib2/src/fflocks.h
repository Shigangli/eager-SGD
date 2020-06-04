#ifndef _FFLOCKS_H_
#define _FFLOCKS_H_

//stolen from Portals 4
# define FFLOCK_TYPE        pthread_spinlock_t
# define FFLOCK_INIT(x)     pthread_spin_init((x), PTHREAD_PROCESS_PRIVATE)
# define FFLOCK_DESTROY(x)  pthread_spin_destroy(x)
# define FFLOCK_LOCK(x)     pthread_spin_lock(x)
# define FFLOCK_TRYLOCK(x)  pthread_spin_trylock(x)
# define FFLOCK_UNLOCK(x)   pthread_spin_unlock(x)

#define FFMUTEX_TYPE pthread_mutex_t
#define FFCOND_TYPE pthread_cond_t

#define FFMUTEX_INIT(x) { pthread_mutex_init(&x, NULL); }
#define FFCOND_INIT(x)  { pthread_cond_init(&x, NULL); }

#define FFMUTEX_DESTROY(x) { pthread_mutex_destroy(&x); }
#define FFCOND_DESTROY(x)  { pthread_cond_destroy(&x); }

#define FFCOND_WAIT(c, m) { \
    pthread_mutex_lock(&m); \
    pthread_cond_wait(&c, &m); \
    pthread_mutex_unlock(&m); \
}

#define FFCOND_SIGNAL(c, m) { \
    pthread_mutex_lock(&m); \
    pthread_cond_broadcast(&c); \
    pthread_mutex_unlock(&m); \
}


#endif // _FFLOCK_H_
