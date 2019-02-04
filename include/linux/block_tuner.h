/*
 * Copyright (C) 2017, Sultanxda <sultanxda@gmail.com>
 * Copyright (C) 2019, Andrzej xNombre Perczak <kartapolska@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 */

#ifndef _BLOCK_TUNER_H
#define _BLOCK_TUNER_H

#ifdef CONFIG_BLOCK_TUNER
extern bool skip_fsync;
extern unsigned int dirty_writeback_interval;
int init_iosched_switcher(struct request_queue *q);
#else
#define skip_fsync 0
static inline int init_iosched_switcher(struct request_queue *q)
{
	return 0;
}
#endif

#endif /* _BLOCK_TUNER_H */
