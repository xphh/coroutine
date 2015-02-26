#include "coroutine.h"
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

#define STACK_SIZE (64<<10)

enum
{
	COROUTINE_NONE = 0,
	COROUTINE_SUSPEND,
	COROUTINE_RUNNING,
	COROUTINE_END,
};

typedef struct coroutine * cort_t;

struct coenviroment
{
	int nco;
	int cap;
	int running;
	cort_t *aco;
	ucontext_t main;
};

struct coroutine
{
	int state;
	coenv_t env;
	coroutine_func func;
	void *context;
	char *stack;
	int stacksize;
	ucontext_t uctx;
};

coenv_t coroutine_init()
{
	struct coenviroment *env = malloc(sizeof(*env));
	env->nco = 0;
	env->cap = 0;
	env->running = -1;
	env->aco = NULL;
	return env;
}

void coroutine_uninit(coenv_t env)
{
	int i;
	for (i = 0; i < env->cap; i++)
	{
		cort_t co = env->aco[i];
		if (co)
		{
			free(co->stack);
			free(co);
		}
	}

	free(env->aco);
	free(env);
}

static int _insert_env(coenv_t env, cort_t co)
{
	if (env->nco >= env->cap)
	{
		int newcap = (env->cap == 0) ? 16 : env->cap * 2;
		env->aco = realloc(env->aco, newcap * sizeof(cort_t));
		memset(env->aco + env->cap, 0, (newcap - env->cap) * sizeof(cort_t));
		env->cap = newcap;
	}

	int i;
	for (i = 0; i < env->cap; i++)
	{
		int id = (i + env->nco) % env->cap;
		if (env->aco[id] == NULL)
		{
			env->aco[id] = co;
			env->nco++;
			return id;
		}
	}

	return -1;
}

static void _delete_coroutine(cort_t co)
{
	free(co->stack);
	free(co);
}

static void _proxyfunc(uint32_t low32, uint32_t hi32)
{
	uintptr_t ptr = (uintptr_t)low32 | ((uintptr_t)hi32 << 32);
	cort_t co = (cort_t)ptr;
	co->func(co->env, co->context);
	co->state = COROUTINE_END;
}

int coroutine_new(coenv_t env, coroutine_func func, void *context)
{
	struct coroutine *co = malloc(sizeof(*co));
	co->state = COROUTINE_SUSPEND;
	co->env = env;
	co->func = func;
	co->context = context;
	co->stacksize = STACK_SIZE;
	co->stack = malloc(co->stacksize);

	getcontext(&co->uctx);
	co->uctx.uc_stack.ss_sp = co->stack;
	co->uctx.uc_stack.ss_size = co->stacksize;
	co->uctx.uc_link = &env->main;

	uintptr_t ptr = (uintptr_t)co;
	makecontext(&co->uctx, (void (*)(void))_proxyfunc, 2, (uint32_t)ptr, (uint32_t)(ptr>>32));

	return _insert_env(env, co);
}

void coroutine_resume(coenv_t env, int id)
{
	if (0 <= id && id < env->nco)
	{
		cort_t co = env->aco[id];
		if (co && co->state == COROUTINE_SUSPEND)
		{
			env->running = id;
			co->state = COROUTINE_RUNNING;
			swapcontext(&co->env->main, &co->uctx);

			if (co->state == COROUTINE_END)
			{
				env->aco[id] = NULL;
				env->nco--;
				env->running = -1;
				_delete_coroutine(co);
			}
		}
	}
}

void coroutine_yield(coenv_t env)
{
	int id = env->running;
	if (0 <= id && id < env->nco)
	{
		cort_t co = env->aco[id];
		if (co && co->state == COROUTINE_RUNNING)
		{
			env->running = -1;
			co->state = COROUTINE_SUSPEND;
			swapcontext(&co->uctx, &co->env->main);
		}
	}
}
