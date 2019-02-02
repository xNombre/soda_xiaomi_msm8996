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

#define WAKELOCK_BLOCKER_VERSION	"1.2.0"

#define LIST_WL_DEFAULT		 "wlan;wlan_wow_wl;wlan_extscan_wl;netmgr_wl;NETLINK;IPA_WS;wlan_ipa;wlan_pno_wl;wcnss_filter_lock"
#define LENGTH_LIST_WL_DEFAULT	 sizeof(LIST_WL_DEFAULT)
#define LENGTH_LIST_WL		 255
#define LENGTH_LIST_WL_SEARCH	 LENGTH_LIST_WL + LENGTH_LIST_WL_DEFAULT + 5

extern void wakeup_source_deactivate(struct wakeup_source *ws);

#ifdef CONFIG_WAKELOCK_BLOCKER
bool is_blocked(struct wakeup_source *ws);
#else
inline bool is_blocked(struct wakeup_source *ws)
{
	return 0;
}
#endif
