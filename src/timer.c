//SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2021 McMCC <mcmcc@mail.ru>
 * timer.c
 */

#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "timer.h"

static time_t start_time = 0;

void timer_start(void)
{
	start_time = time(0);
}

int timer_txfr_speed(uint32_t txfred)
{
	time_t now = 0, elapsed = 0;

	if (!txfred)
		return 0;

	time(&now);
	elapsed = difftime(now, start_time);

	if (!elapsed)
		return 0;

	return txfred / elapsed;
}


void timer_end(void)
{
	time_t end_time = 0, elapsed_seconds = 0;

	time(&end_time);
	elapsed_seconds = difftime(end_time, start_time);
	printf("Elapsed time: %d seconds\n", (int)elapsed_seconds);
}
