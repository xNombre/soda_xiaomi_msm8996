/*
 * Author: andip71, 01.09.2017
 * Enhanted by xNombre (Andrzej Perczak)
 *
 * Version 1.2.0
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*
 * Change log:
 *
 * 1.2.0 (01.02.2019)
 *   - Fix a terrible mess that was done here before, I'd blame at its
 *     initial creator but just be polite here...
 *     Also add pm notifier to switch off blocker when screen is on.
 *
 * 1.1.0 (01.09.2017)
 *   - By default, the following wakelocks are blocked in an own list
 *     qcom_rx_wakelock, wlan, wlan_wow_wl, wlan_extscan_wl, NETLINK
 *
 * 1.0.1 (29.08.2017)
 *   - Add killing wakelock when currently active
 *
 * 1.0.0 (28.08.2017)
 *   - Initial version
 *
 */

#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/fb.h>
#include "boeffla_wl_blocker.h"

char list_wl[LENGTH_LIST_WL];
char list_wl_default[LENGTH_LIST_WL_DEFAULT];
char __read_mostly list_wl_search[LENGTH_LIST_WL_SEARCH];

static bool __read_mostly wl_blocker_active;
static struct notifier_block fb_notifier;

bool is_blocked(struct wakeup_source *ws)
{

	if (!wl_blocker_active)
		return 0;

	if(!strstr(list_wl_search, ws->name))
		return 0;

	// if it is currently active, deactivate it
	if (ws->active)
		wakeup_source_deactivate(ws);

	return 1;
}

static void build_search_string(char *list1, char *list2)
{
	sprintf(list_wl_search, "%s;%s", list1, list2);
}

static int fb_notifier_callback(struct notifier_block *nb,
				unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank;

	if (evdata && evdata->data && event == FB_EVENT_BLANK) {
		blank = evdata->data;
		switch(*blank) {
			case FB_BLANK_UNBLANK:
				wl_blocker_active = false;
				break;
			case FB_BLANK_POWERDOWN:
				wl_blocker_active = true;
				break;
		}
	}

	return 0;
}

static ssize_t wakelock_blocker_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	return sprintf(buf, "%s\n", list_wl);
}

static ssize_t wakelock_blocker_store(struct device * dev, struct device_attribute *attr,
			     const char * buf, size_t n)
{
	int len = n;

	if (len > LENGTH_LIST_WL)
		return -EINVAL;

	sscanf(buf, "%s", list_wl);
	build_search_string(list_wl_default, list_wl);

	return n;
}

static ssize_t wakelock_blocker_default_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	return sprintf(buf, "%s\n", list_wl_default);
}

static ssize_t wakelock_blocker_default_store(struct device * dev, struct device_attribute *attr,
			     const char * buf, size_t n)
{
	int len = n;

	if (len > LENGTH_LIST_WL_DEFAULT)
		return -EINVAL;

	sscanf(buf, "%s", list_wl_default);
	build_search_string(list_wl_default, list_wl);

	return n;
}

static ssize_t version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", BOEFFLA_WL_BLOCKER_VERSION);
}

static DEVICE_ATTR(wakelock_blocker, 0644, wakelock_blocker_show, wakelock_blocker_store);
static DEVICE_ATTR(wakelock_blocker_default, 0644, wakelock_blocker_default_show, wakelock_blocker_default_store);
static DEVICE_ATTR(version, 0444, version_show, NULL);

static struct attribute *boeffla_wl_blocker_attributes[] = {
	&dev_attr_wakelock_blocker.attr,
	&dev_attr_wakelock_blocker_default.attr,
	&dev_attr_version.attr,
	NULL
};

static struct attribute_group boeffla_wl_blocker_control_group = {
	.attrs = boeffla_wl_blocker_attributes,
};

static struct miscdevice boeffla_wl_blocker_control_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "boeffla_wakelock_blocker",
};

static int __init boeffla_wl_blocker_init(void)
{
	int ret;

	fb_notifier.notifier_call = fb_notifier_callback;
	ret = fb_register_client(&fb_notifier);
	if (ret) {
		pr_err("Boeffla WL blocker: unable to register fb_notifier.\n");
		return ret;
	}

	ret = misc_register(&boeffla_wl_blocker_control_device);
	if (ret) {
		pr_err("Boeffla WL blocker: failed to register misc.\n");
		return ret;
	}

	ret = sysfs_create_group(&boeffla_wl_blocker_control_device.this_device->kobj,
				&boeffla_wl_blocker_control_group);
	if (ret) {
		pr_err("Boeffla WL blocker: failed to create sys fs object.\n");
		return ret;
	}

	sprintf(list_wl_default, "%s" , LIST_WL_DEFAULT);
	build_search_string(list_wl_default, list_wl);

	pr_info("Boeffla WL blocker: driver version %s started\n", BOEFFLA_WL_BLOCKER_VERSION);

	return 0;
}

module_init(boeffla_wl_blocker_init);
