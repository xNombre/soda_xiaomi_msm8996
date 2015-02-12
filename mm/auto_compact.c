/* Copyright (C) 2019 Andrzej Perczak (xNombre) kartapolska@gmail.com
 * Simple memory compaction module which triggers after screen is turned off.
 * This module is inspired by Alex Naidis (TheCrazyLex) <alex.naidis@linux.com> - Thank you!
 */

#include <linux/fb.h>
#include <linux/moduleparam.h>
#include <linux/time.h>
#include <linux/workqueue.h>
#include "internal.h"

#define COMPACTION_TIMEOUT 1800
#define TRIGGER_AFTER 5

static struct delayed_work compaction_work;
static unsigned long compaction_triggered_last;

static int fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank;

	if ((event == FB_EVENT_BLANK) && evdata && evdata->data) {
		blank = evdata->data;

		switch (*blank) {
		case FB_BLANK_POWERDOWN:
			if (time_after(jiffies, compaction_triggered_last) && !delayed_work_pending(&compaction_work)) {
				queue_delayed_work(system_freezable_power_efficient_wq, &compaction_work,
					msecs_to_jiffies(TRIGGER_AFTER));
			}
		break;
		case FB_BLANK_UNBLANK:
			cancel_delayed_work(&compaction_work);
		break;
		}
	}

	return 0;
}

static struct notifier_block compaction_notifier_block = {
	.notifier_call = fb_notifier_callback,
};

static void do_compaction(struct work_struct *work)
{
	pr_debug("Scheduled memory compaction is starting");

	/* Do full compaction */
	compact_nodes();

	/* Do not trigger next compaction before timeout */
	compaction_triggered_last = jiffies + msecs_to_jiffies(COMPACTION_TIMEOUT);

	pr_debug("Scheduled memory compaction is completed");
}

static int  __init scheduled_compaction_init(void)
{
	int ret;

	INIT_DELAYED_WORK(&compaction_work, do_compaction);

	ret = fb_register_client(&compaction_notifier_block);
	if(!ret)
		pr_debug("auto_compat: initialized!");

	return ret;
}
late_initcall(scheduled_compaction_init);
