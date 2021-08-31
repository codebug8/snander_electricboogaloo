//SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2021 McMCC <mcmcc@mail.ru>
 * timer.h
 */
#ifndef __TIMER_H__
#define __TIMER_H__

void timer_start(void);
int timer_txfr_speed(uint32_t txfred);
void timer_end(void);

#endif /* __TIMER_H__ */
