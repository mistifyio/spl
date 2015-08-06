/*****************************************************************************\
 *  Copyright 2015 OmniTI Computer Consulting, Inc. All rights reserved.
 *  Written by Albert Lee <trisk@omniti.com>.
 *
 *  This file is part of the SPL, Solaris Porting Layer.
 *  For details, see <http://zfsonlinux.org/>.
 *
 *  The SPL is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  The SPL is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with the SPL.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************
 *  Solaris Porting Layer (SPL) Zones Implementation.
\*****************************************************************************/

#include <sys/zone.h>
#include <sys/mutex.h>
#include <sys/list.h>
#include <sys/kmem.h>
#include <linux/nsproxy.h>
#include <linux/dataset_namespace.h>

static kmutex_t zone_lock;
static struct list_head zone_list;

zone_t *
crgetzone(struct task_struct *task)
{
	struct dataset_namespace *dsns;
	zone_t *zone = NULL;

	dsns = task->nsproxy->dataset_ns;
	mutex_enter(&zone_lock);
	list_for_each_entry(zone, &zone_list, zone_list)
		if (zone->zone_namespace == dsns)
			break;
	mutex_exit(&zone_lock);

	return (zone);
}
EXPORT_SYMBOL(crgetzone);

zoneid_t
crgetzoneid(struct task_struct *task)
{
	zone_t *zone;

	zone = crgetzone(task);
	if (zone == NULL)
		return ((zoneid_t)-1);

	return (zone->zone_id);
}
EXPORT_SYMBOL(crgetzoneid);

static void
zone_free_datasets(zone_t *zone)
{
	zone_dataset_t *zd, *next;

	for (zd = list_head(&zone->zone_datasets); zd != NULL; zd = next) {
		next = list_next(&zone->zone_datasets, zd);
		list_remove(&zone->zone_datasets, zd);
		kmem_free(zd->zd_dataset, strlen(zd->zd_dataset) + 1);
		kmem_free(zd, sizeof(zone_dataset_t));
	}
	list_destroy(&zone->zone_datasets);
}

int
zone_dataset_visible(const char *dataset, int *writable)
{
	zone_t *zone;
	zone_dataset_t *zd, *next;
	int len;

	if (dataset == NULL || *dataset == '\0')
		return (0);

	zone = crgetzone(curproc);
	if (zone == NULL)
		return (0);

	if (zone->zone_id == GLOBAL_ZONEID)
	{
		/* Global zone */
		if (writable != NULL)
			*writable = 1;
		return (1);
	}

#if 1
	/*
	 * Demo shit.
	 */
	if ((strncmp(dataset, "mistify/guests/", 15) == 0) ||
	    strncmp(dataset, "mistify/demo/", 13) == 0) {
		if (writable != NULL)
			*writable = 1;
		return (1);
	}
	return (0);
#endif
	/*
	 * Search for parents of this dataset or exact matches. If found, expose this
	 * dataset as writable.
	 */
	for (zd = list_head(&zone->zone_datasets); zd != NULL; zd = next) {
		next = list_next(&zone->zone_datasets, zd);
		len = strlen(zd->zd_dataset);
		if (strncmp(dataset, zd->zd_dataset, len) == 0) {
			switch (dataset[len]) {
			case '\0':
			case '/':
			case '@':
				if (writable != NULL)
					*writable = 1;
				return (1);
			}
		}
	}

	/*
	 *  Search for children of this dataset. If found, expose this dataset as
	 *  read-only.
	 */
	len = strlen(dataset);
	if (dataset[len - 1] == '/')
		len--;
	for (zd = list_head(&zone->zone_datasets); zd != NULL; zd = next) {
		next = list_next(&zone->zone_datasets, zd);
		if (strncmp(dataset, zd->zd_dataset, len) == 0 &&
		    zd->zd_dataset[len] == '/') {
			if (writable != NULL)
				*writable = 0;
			return (1);
		}
	}

	return (0);
}
EXPORT_SYMBOL(zone_dataset_visible);

static int
zone_init(struct dataset_namespace *ns)
{
	zone_t *zone;

	zone = kmem_alloc(sizeof (zone_t), KM_SLEEP);
	zone->zone_id = ns->ns.inum;
	if (ns == &init_dataset_ns)
		zone->zone_id = GLOBAL_ZONEID;
	zone->zone_namespace = ns;
	list_create(&zone->zone_datasets, sizeof (zone_dataset_t),
	    offsetof(zone_dataset_t, zd_linkage));
	mutex_enter(&zone_lock);
	list_add_tail(&zone->zone_list, &zone_list);
	mutex_exit(&zone_lock);

	return (0);
}


static void
zone_exit(struct dataset_namespace *ns)
{
	zone_t *zone;

	mutex_enter(&zone_lock);
	list_for_each_entry(zone, &zone_list, zone_list)
		if (zone->zone_namespace == ns)
			break;
	list_del(&zone->zone_list);
	mutex_exit(&zone_lock);
	zone_free_datasets(zone);
	kmem_free(zone, sizeof (zone_t));
}

static struct dataset_operations zone_operations = {
	.init = zone_init,
	.exit = zone_exit,
};

int
spl_zone_init(void)
{
	int error = 0;

	mutex_init(&zone_lock, NULL, MUTEX_DEFAULT, NULL);
	INIT_LIST_HEAD(&zone_list);
	error = register_dataset_provider(&zone_operations);
	if (error != 0)
		mutex_destroy(&zone_lock);

	return (error);
}

void
spl_zone_fini(void)
{
	unregister_dataset_provider(&zone_operations);
	mutex_destroy(&zone_lock);
}

