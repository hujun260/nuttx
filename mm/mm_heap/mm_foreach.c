/****************************************************************************
 * mm/mm_heap/mm_foreach.c
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

#include <nuttx/config.h>

#include <assert.h>
#include <debug.h>

#include <nuttx/mm/mm.h>

#include "mm_heap/mm.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mm_foreach
 *
 * Description:
 *   Visit each node to run handler in heap.
 *
 ****************************************************************************/

void mm_foreach(FAR struct mm_heap_s *heap, mm_node_handler_t handler,
                FAR void *arg)
{
  FAR struct mm_allocnode_s *node;
  FAR struct mm_allocnode_s *prev;
  irqstate_t flags;
  size_t nodesize;

#if CONFIG_MM_REGIONS > 1
  int region;
#else
#  define region 0
#endif

  DEBUGASSERT(handler);

  /* Visit each region */

#if CONFIG_MM_REGIONS > 1
  for (region = 0; region < heap->mm_nregions; region++)
#endif
    {
      prev = NULL;

      /* Visit each node in the region
       * Retake the mutex for each region to reduce latencies
       */

      if (_SCHED_GETTID() < 0)
        {
          return;
        }

      flags = spin_lock_irqsave(&heap->mm_lock);
      for (node = heap->mm_heapstart[region];
           node < heap->mm_heapend[region];
           node = (FAR struct mm_allocnode_s *)((FAR char *)node + nodesize))
        {
          nodesize = MM_SIZEOF_NODE(node);
          minfo("region=%d node=%p size=%zu preceding=%u (%c %c)\n",
                region, node, nodesize, (unsigned int)node->preceding,
                MM_PREVNODE_IS_FREE(node) ? 'F' : 'A',
                MM_NODE_IS_ALLOC(node) ? 'A' : 'F');

          handler(node, arg);

          DEBUGASSERT(MM_PREVNODE_IS_ALLOC(node) ||
                      MM_SIZEOF_NODE(prev) == node->preceding);
          prev = node;
        }

      minfo("region=%d node=%p heapend=%p\n",
            region, node, heap->mm_heapend[region]);
      DEBUGASSERT(node == heap->mm_heapend[region]);
      handler(node, arg);

      spin_unlock_irqrestore(&heap->mm_lock, flags);
    }
#undef region
}
