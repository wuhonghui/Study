#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "common.h"
#include "utils/timing.h"

void simple_timing_begin(struct timeval *begin) {
    gettimeofday(begin, NULL);
}

int simple_timing_end(struct timeval *begin)
{
    struct timeval end;
    unsigned int t = 0;

	gettimeofday(&end, NULL);
    t = end.tv_usec - begin->tv_usec;
    if (end.tv_sec > begin->tv_sec)
    {
        t += (1000000 * (end.tv_sec - begin->tv_sec));
    }
    printf("time cost: %u us\n", t);
	return t;
}

/* a-b=c */
void timeval_sub(struct timeval *a, struct timeval *b, struct timeval *c)
{
	c->tv_sec = a->tv_sec - b->tv_sec;

	if (a->tv_usec < b->tv_usec) {
        c->tv_sec--;
        c->tv_usec = (a->tv_usec - b->tv_usec) + 1000000;
    } else {
        c->tv_usec = (a->tv_usec - b->tv_usec);
    }
}

/* c = a + b */
void timeval_add(struct timeval *a, struct timeval *b, struct timeval *c)
{
	c->tv_sec = a->tv_sec + b->tv_sec;
	c->tv_sec += (a->tv_usec + b->tv_usec) / 1000000;
	c->tv_usec = (a->tv_usec + b->tv_usec) % 1000000;
}

float timeval_to_float(struct timeval *a)
{
	float b;
	b = (float)a->tv_sec;
	b += (float)(a->tv_usec / 1000000);
	return b;
}

void timeval_to_str(struct timeval *t, char *buf, int bufsize)
{
	struct tm *nowtm = NULL;
	char tmbuf[64];

	nowtm = gmtime(&(t->tv_sec));
	strftime(tmbuf, sizeof(tmbuf), "%Y-%m-%d %Z %H:%M:%S", nowtm);
	snprintf(buf, bufsize, "%s.%06d", tmbuf, (int)t->tv_usec);
}

struct timing_helper {
	struct timeval init;
	struct timeval begin;
	struct timeval total;
	int count;
	char name[24];
};

struct timing_helper * timing_helper_new(char *name)
{
	struct timing_helper *t = (struct timing_helper *)malloc(sizeof(struct timing_helper));
	memset(t, 0, sizeof(struct timing_helper));
	strcpy(t->name, name);
	gettimeofday(&(t->init), NULL);
	return t;
}


void timing_helper_reset(struct timing_helper *t)
{
	gettimeofday(&(t->init), NULL);
	t->count = 0;
	t->begin.tv_sec = 0;
	t->begin.tv_usec = 0;
	t->total.tv_sec = 0;
	t->total.tv_usec = 0;
}

void timing_helper_begin(struct timing_helper *t)
{
	gettimeofday(&(t->begin), NULL);
}

void timing_helper_end(struct timing_helper *t)
{
	struct timeval end;
	struct timeval snip;
	gettimeofday(&end, NULL);

	timeval_sub(&end, &t->begin, &snip);
	timeval_add(&t->total, &snip, &t->total);
	t->count++;
}

void timing_helper_show(struct timing_helper *t)
{
	struct timeval end;

	gettimeofday(&end, NULL);
	timeval_sub(&end, &t->init, &end);

	printf("Timing %s in %d.%06ds\n", t->name, (int)end.tv_sec, (int)end.tv_usec);
	printf("Called %d times in %d.%06ds\n\n", t->count, (int)t->total.tv_sec, (int)t->total.tv_usec);
}

void timing_helper_free(struct timing_helper *t)
{
	timing_helper_show(t);
	free(t);
}



