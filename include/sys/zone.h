/*****************************************************************************\
 *  Copyright 2015 OmniTI Computer Consulting, Inc. All rights reserved.
 *  Copyright (C) 2007-2010 Lawrence Livermore National Security, LLC.
 *  Copyright (C) 2007 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Brian Behlendorf <behlendorf1@llnl.gov>.
 *  UCRL-CODE-235197
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
\*****************************************************************************/

#ifndef _SPL_ZONE_H
#define _SPL_ZONE_H

#include <sys/byteorder.h>
#include <sys/list.h>
#include <sys/types.h>

#define	GLOBAL_ZONEID			0

#define	INGLOBALZONE(z)			(crgetzoneid(z) == GLOBAL_ZONEID)

struct dataset_namespace;

typedef unsigned long zoneid_t;

typedef struct spl_zone {
	zoneid_t    zone_id;
	list_node_t zone_list;
	struct dataset_namespace   *zone_namespace;
	list_t	    zone_datasets;
} zone_t;

typedef struct zone_dataset {
	char	    *zd_dataset;
	list_node_t zd_linkage;
} zone_dataset_t;

extern zone_t * crgetzone(struct task_struct *task);
extern zoneid_t crgetzoneid(struct task_struct *task);

extern int zone_dataset_visible(const char *dataset, int *writable);

int spl_zone_init(void);
void spl_zone_fini(void);

#endif /* SPL_ZONE_H */
