#include "coroutine.h"
#include <stdio.h>

typedef struct  
{
	int id;
} myctx;

static void cofunc(coenv_t env, void *context)
{
	myctx *ctx = (myctx *)context;
	int i;
	for (i = 0; i < 5; i++)
	{
		printf("env[%p] ctx[%d] i = %d\n", env, ctx->id, i);
		coroutine_yield(env);
	}
}

int main()
{
	coenv_t env = coroutine_init();

	myctx ctx1 = {1};
	myctx ctx2 = {2};
	myctx ctx3 = {3};

	int co1 = coroutine_new(env, cofunc, &ctx1);
	printf("coroutine_new %d\n", co1);
	int co2 = coroutine_new(env, cofunc, &ctx2);
	printf("coroutine_new %d\n", co2);
	int co3 = coroutine_new(env, cofunc, &ctx3);
	printf("coroutine_new %d\n", co3);

	int i;
	for (i = 0; i < 10; i++)
	{
		coroutine_resume(env, co1);
		coroutine_resume(env, co2);
		coroutine_resume(env, co3);
	}

	coroutine_uninit(env);

	return 0;
}
