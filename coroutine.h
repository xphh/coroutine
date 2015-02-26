#ifndef _COROUTINE_H_
#define _COROUTINE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct coenviroment * coenv_t;

coenv_t coroutine_init();
void coroutine_uninit(coenv_t);

typedef void (*coroutine_func)(coenv_t, void *context);

int coroutine_new(coenv_t, coroutine_func, void *context);

void coroutine_resume(coenv_t, int id);
void coroutine_yield(coenv_t);

#ifdef __cplusplus
}
#endif

#endif
