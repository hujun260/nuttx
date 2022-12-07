/****************************************************************************
 * drivers/binder/binder_sched.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#define LOG_TAG  "BinderSched"

#include <nuttx/config.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <string.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <debug.h>
#include <sched.h>
#include <nuttx/fs/fs.h>
#include <nuttx/android/binder.h>
#include <nuttx/mutex.h>
#include <nuttx/nuttx.h>
#include <nuttx/kmalloc.h>
#include <nuttx/semaphore.h>
#include <nuttx/wqueue.h>

#include "binder_internal.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int binder_get_priority(pid_t pid, FAR struct binder_priority * priority)
{
  int                   ret;
  struct sched_param    params;

  ret                   = sched_getparam(pid, &params);
  priority->sched_prio  = params.sched_priority;
  if (ret != 0)
    {
      return ret;
    }

  priority->sched_policy = sched_getscheduler(pid);
  if (priority->sched_policy < 0)
    {
      return priority->sched_policy;
    }

  return 0;
}

static int binder_set_priority_internal(pid_t pid, unsigned int sched_policy,
                                        int prio)
{
  struct sched_param    params;
  int                   ret;

  params.sched_priority = prio;
  ret                   = sched_setscheduler(pid, sched_policy, &params);

  binder_debug(BINDER_DEBUG_PRIORITY, "pid=%d, ret=%d\n", pid, ret);
  return ret;
}

void binder_set_priority(FAR struct binder_thread *thread,
                         FAR const struct binder_priority *desired)
{
  int                       priority;
  unsigned int              policy;
  struct binder_priority    task_priority;

  binder_get_priority(thread->tid, &task_priority);

  if (task_priority.sched_policy == desired->sched_policy &&
      task_priority.sched_prio == desired->sched_prio)
    {
      return;
    }

  policy    = desired->sched_policy;
  priority  = desired->sched_prio;
  binder_set_priority_internal(thread->tid, policy, priority);
}

void init_waitqueue_entry(FAR struct wait_queue_entry *wq_entry,
                          FAR void * arg, wait_queue_func_t func)
{
  wq_entry->private = arg;
  wq_entry->func    = func;
  list_initialize(&wq_entry->entry);

  binder_debug(BINDER_DEBUG_SCHED, "wq_entry=%p\n", wq_entry);
}

void prepare_to_wait(FAR struct list_node *wq_head,
                     FAR struct wait_queue_entry *wq_entry)
{
  irqstate_t flags;

  flags = enter_critical_section();
  if (list_is_empty(&wq_entry->entry))
    {
      list_add_tail(wq_head, &wq_entry->entry);
    }

  leave_critical_section(flags);
  binder_debug(BINDER_DEBUG_SCHED, "wq_head=%p, wq_entry=%p\n", wq_head,
               wq_entry);
}

void finish_wait(FAR struct wait_queue_entry *wq_entry)
{
  irqstate_t flags;

  binder_debug(BINDER_DEBUG_SCHED, "wq_entry=%p\n", wq_entry);

  flags = enter_critical_section();
  if (!list_is_empty(&wq_entry->entry))
    {
      list_delete_init(&wq_entry->entry);
      wq_entry->private = 0;
      wq_entry->func    = 0;
    }

  leave_critical_section(flags);
}

void wait_wake_up(FAR struct list_node *wq_head, int sync)
{
  struct wait_queue_entry   *curr;
  struct wait_queue_entry   *next;
  int                        ret;

  list_for_every_entry_safe(wq_head, curr, next,
                            struct wait_queue_entry, entry)
    {
      ret = curr->func(curr, sync);
      if (ret < 0)
        {
          break;
        }
    }
}

void wake_up_pollfree(FAR struct binder_thread *thread)
{
  FAR struct wait_queue_entry  *wq_entry;

  wq_entry  = &thread->wq_entry;

  if (wq_entry->func)
    {
      wq_entry->func(wq_entry, 0);
    }

  binder_debug(BINDER_DEBUG_SCHED, "%d:%d wake up\n",
               thread->tid, thread->proc->pid);
}
