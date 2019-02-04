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

#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/writeback.h>
#include <linux/block_tuner.h>
#include <linux/fb.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/blk_types.h>
#include <linux/elevator.h>

/**********************        TUNABLES         **********************/
// screen notif timeout is 10s so let's assume safe value to prevent any juggling
#define RESUME_TIMEOUT_MSECS 		11000

#define SCREEN_ON_DIRTY_WRITEBACK_CENTISECS 	900
#define SCREEN_OFF_DIRTY_WRITEBACK_CENTISECS 	2900

#define SCREEN_OFF_IOSCHED 			"noop"
/*********************************************************************/

bool skip_fsync __read_mostly = false;
static bool block_tuner_enabled = true;
static bool screen_state = 1;

static DEFINE_SPINLOCK(init_lock);
static DEFINE_SPINLOCK(kobj_lock);

static struct notifier_block fb_notifier;
static struct delayed_work resume_worker;
static struct kobject *dyn_fsync_kobj;
static struct kobject *block_tuner_kobj;

struct req_queue_data {
	struct list_head list;
	struct request_queue *queue;
	char prev_e[ELV_NAME_MAX];
};

static struct req_queue_data req_queues = {
	.list = LIST_HEAD_INIT(req_queues.list),
};

static void change_elevator(struct req_queue_data *r)
{
	struct request_queue *q = r->queue;

	if (screen_state) {
		elevator_change(q, r->prev_e);
	} else {
		strcpy(r->prev_e, q->elevator->type->elevator_name);
		elevator_change(q, SCREEN_OFF_IOSCHED);
	}
}

static void change_all_elevators(struct list_head *head)
{
	struct req_queue_data *r;

	list_for_each_entry(r, head, list)
		change_elevator(r);
}

int init_iosched_switcher(struct request_queue *q)
{
	struct req_queue_data *r;

	r = kzalloc(sizeof(*r), GFP_KERNEL);
	if (!r)
		return -ENOMEM;

	r->queue = q;

	spin_lock(&init_lock);
	list_add(&r->list, &req_queues.list);
	spin_unlock(&init_lock);

	return 0;
}

static void emergency_event(void)
{
	cancel_delayed_work(&resume_worker);
	skip_fsync = false;
	emergency_sync();
	pr_warn("Block tuner: Emergency event - force flush!\n");
}

static int tuner_panic_event(struct notifier_block *this,
			     unsigned long event, void *ptr)
{
	if(skip_fsync)
		emergency_event();

	return NOTIFY_DONE;
}


static int tuner_notify_sys(struct notifier_block *this, unsigned long code,
				void *unused)
{
	if (skip_fsync && (code == SYS_DOWN || code == SYS_HALT))
		emergency_event();

	return NOTIFY_DONE;
}

static struct notifier_block tuner_panic_block = 
{
	.notifier_call  = tuner_panic_event,
	.priority       = INT_MAX,
};

static struct notifier_block tuner_notifier_sys = 
{
	.notifier_call = tuner_notify_sys,
};

static void apply_parameters(void)
{
	change_all_elevators(&req_queues.list);

	if(screen_state) {
		dirty_writeback_interval = SCREEN_ON_DIRTY_WRITEBACK_CENTISECS;
		skip_fsync = true;
	} else {
		dirty_writeback_interval = SCREEN_OFF_DIRTY_WRITEBACK_CENTISECS;
		skip_fsync = false;
	}
}

static void resume_work(struct work_struct *work)
{
	screen_state = 1;

	apply_parameters();
}

static int fb_notifier_callback(struct notifier_block *this,
				unsigned long event, void *data)
{
 	struct fb_event *evdata = data;
	int *blank;

	if(!block_tuner_enabled)
		return 0;

  	if (evdata && evdata->data && event == FB_EVENT_BLANK) {
  		blank = evdata->data;
  		switch (*blank) {
  			case FB_BLANK_UNBLANK:
				schedule_delayed_work(&resume_worker, msecs_to_jiffies(RESUME_TIMEOUT_MSECS));
  				break;

  			case FB_BLANK_POWERDOWN:
		/*
		 * Switch to noop when the screen turns off. Purposely block
		 * the fb notifier chain call in case weird things can happen
		 * when switching elevators while the screen is off.
		 */
				cancel_delayed_work_sync(&resume_worker);

				if(!screen_state)
					return 0;

				screen_state=0;
				apply_parameters();
  				break;
  		}
	}
 
 	return 0;
}

static struct notifier_block fb_notifier = 
{
	.notifier_call = fb_notifier_callback,
};

static ssize_t block_tuner_active_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
 	return sprintf(buf, "%d\n", block_tuner_enabled);
}

static ssize_t block_tuner_active_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t count)
{
	int i;

	if (sscanf(buf, "%u", &i) == 1 && i < 2) {
		spin_lock(&kobj_lock);
		block_tuner_enabled = (i == 1);

		// Restore default settings
		if(!block_tuner_enabled && !screen_state) {
			cancel_delayed_work_sync(&resume_worker);
			schedule_delayed_work(&resume_worker, msecs_to_jiffies(0));
		}
		spin_unlock(&kobj_lock);

		return count;
	} else
		return -EINVAL;
}

// This is left to provide compability with stock kernel managers
static struct kobj_attribute dyn_fsync_active_attribute = 
	__ATTR(Dyn_fsync_active, 0644,
		block_tuner_active_show,
		block_tuner_active_store);
		
static struct attribute *dyn_fsync_active_attrs[] =
{
	&dyn_fsync_active_attribute.attr,
	NULL,
};

static struct attribute_group dyn_fsync_attr_group =
{
	.attrs = dyn_fsync_active_attrs,
};

static struct kobj_attribute block_tuner_active_attribute =
	__ATTR(block_tuner_enabled, 0644,
		block_tuner_active_show,
		block_tuner_active_store);

static struct attribute *block_tuner_attrs[] =
{
	&block_tuner_active_attribute.attr,
	NULL,
};

static struct attribute_group block_tuner_attr_group =
{
	.attrs = block_tuner_attrs,
};

static int __init block_tuner_init(void)
{
	int ret;

	dyn_fsync_kobj = kobject_create_and_add("dyn_fsync", kernel_kobj);
	if (!dyn_fsync_kobj) 
	{
		pr_err("%s: dyn_fsync_kobj create failed!", __FUNCTION__);
		return -ENOMEM;
	}

	ret = sysfs_create_group(dyn_fsync_kobj, &dyn_fsync_attr_group);
	if (ret) 
	{
		pr_err("%s: dyn_fsync sysfs create failed!", __FUNCTION__);
		goto err1;
	}

	block_tuner_kobj = kobject_create_and_add("block_tuner", kernel_kobj);
	if (!block_tuner_kobj) 
	{
		pr_err("%s: block_tuner_kobj create failed!", __FUNCTION__);
		ret = -ENOMEM;
		goto err1;
	}

	ret = sysfs_create_group(block_tuner_kobj, &block_tuner_attr_group);
	if (ret) 
	{
		pr_err("%s: block_tuner_kobj sysfs create failed!", __FUNCTION__);
		goto err2;
	}

	ret = fb_register_client(&fb_notifier);
	if(ret) {
		pr_err("%s: failed to register fb notifier!", __FUNCTION__);
		goto err2;
	}

	register_reboot_notifier(&tuner_notifier_sys);
	atomic_notifier_chain_register(&panic_notifier_list, &tuner_panic_block);
	INIT_DELAYED_WORK(&resume_worker, resume_work);

	return ret;

err2:
	kobject_put(block_tuner_kobj);
err1:
	kobject_put(dyn_fsync_kobj);
	return ret;
}

late_initcall(block_tuner_init);
