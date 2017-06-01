#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	MaxGThreads = 16,
    MaxGTQueue = 2000000,
	StackSize = 0x400000,
};

typedef void(*func)(void);

struct gt {
    long long id;
	enum {
		Unused,
		Running,
		Ready,
	} st;
    char* stack;
    func f;
    struct gtctx {
        uint64_t rsp;
        uint64_t r15;
        uint64_t r14;
        uint64_t r13;
        uint64_t r12;
        uint64_t rbx;
        uint64_t rbp;
    } ctx;
};

struct gt gttbl[MaxGThreads];
struct gt *gtcur;
struct gt *gtqueue = 0;
struct gt *gtqueue_tail = 0;

void gtswtch2(struct gtctx *oldt, struct gtctx *newt)
{
    int x = oldt->rsp;
    int y = oldt->rbp;
    newt->rsp = x;
    newt->rbp = y;
}

void gtinit(void);
void gtret(int ret);
void gtswtch(struct gtctx *oldt, struct gtctx *newt);
bool gtyield(void);
static void gtstop(void);
int gtgo(long long id, func f);

void
__attribute__((noinline))
gtinit(void)
{
    struct gt *p;
	gtcur = &gttbl[0];
	gtcur->st = Running;
    gtcur->stack = malloc(StackSize);
    gtcur->id = 0;
    gtqueue = malloc(sizeof(struct gt)*MaxGTQueue);
    gtqueue_tail = &gtqueue[0];
    
    for (p = &gttbl[1]; p != &gttbl[MaxGThreads]; p++)
    {
        p->st = Unused;
        p->stack = malloc(StackSize);
    }

    for (p = &gtqueue[0]; p != &gtqueue[MaxGTQueue]; p++)
    {
        p->st = Unused;
        p->stack = 0;
    }
}


void
__attribute__((noreturn))
__attribute__((noinline))
gtret(int ret)
{
	if (gtcur != &gttbl[0]) {
		gtcur->st = Unused;
		gtyield();
		assert(!"reachable");
	}
	while (gtyield())
		;
	exit(ret);
}

void
pop(struct gt *p, struct gt *q)
{
    memset(p->stack, StackSize, 0);

    *(uint64_t *)&p->stack[StackSize - 8] = (uint64_t)gtstop;
    *(uint64_t *)&p->stack[StackSize - 16] = (uint64_t)q->f;
    p->ctx.rsp = (uint64_t)&p->stack[StackSize - 16];
    p->st = Ready;
    p->id = q->id;
    q->st = Unused;
}

bool
__attribute__((noinline))
gtyield(void)
{
	struct gt *p, *q;
    struct gt temp;
	struct gtctx *oldt, *newt;

    for (p = &gttbl[1]; p != &gttbl[MaxGThreads]; p++)
    {
        if (p != gtcur && p->st == Unused)
            for (q = &gtqueue[0]; q != gtqueue_tail; q++)
            {
                if (q->st == Ready)
                {
                    pop(p, q);
                    if (&gtqueue[0] != gtqueue_tail)
                    {
                        --gtqueue_tail;
                    }
                    if (q != gtqueue_tail)
                    {
                        temp = *q;
                        *q = *gtqueue_tail;
                        *p = temp;
                    }
                    break;
                }
            }
    }

	p = gtcur;
	while (p->st != Ready) {
		if (++p == &gttbl[MaxGThreads])
			p = &gttbl[0];
		if (p == gtcur)
			return false;
	}

	if (gtcur->st != Unused)
		gtcur->st = Ready;
	p->st = Running;
	oldt = &gtcur->ctx;
	newt = &p->ctx;
    //printf("Switch %lld->%lld\n", gtcur->id, p->id);
	gtcur = p;    
	gtswtch(oldt, newt);
	return true;
}

static void
__attribute__((noinline))
gtstop(void) { gtret(0); }

int
__attribute__((noinline))
gtgo(long long id, func f)
{
	struct gt *p = gtqueue_tail++;
    if (p == &gtqueue[MaxGTQueue])
    {
        --gtqueue_tail;
        printf("Queue overflow on %lld\n", id);
        return -1;
    }

    p->st = Ready;
    p->f = f;
    p->id = id;
    //printf("Queued #%lld\n", id);

	return 0;
}

/* Now, let's run some simple threaded code. */

void
f(void)
{
	//static int x;
	int i, id;

	//id = x++;
    id = gtcur->id;
    printf("SN GT(%d): started\n", id);
    gtyield();
	for (i = 0; i < 4; i++) {
		printf("SN GT(%d): printf %d\n", id, i);
		gtyield();
	}
    printf("SN GT(%d): finished\n", id);
}

int
main(void)
{
    int i = 0;
	gtinit();
    for (i = 0; i < MaxGTQueue; ++i)
    {
        if (gtgo(i, f) == -1)
        {            
        }
        gtyield();
    }
    //f();
	gtret(1);
}
