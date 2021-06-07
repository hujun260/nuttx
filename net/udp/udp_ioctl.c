/****************************************************************************
 * net/udp/udp_ioctl.c
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

#include <stdint.h>
#include <stdbool.h>
#include <debug.h>

#include <net/if.h>

#include <nuttx/fs/ioctl.h>
#include <nuttx/mm/iob.h>

#include "udp/udp.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: udp_ioctl
 *
 * Description:
 *   This function performs udp specific ioctl() operations.
 *
 * Parameters:
 *   conn     The TCP connection of interest
 *   cmd      The ioctl command
 *   arg      The argument of the ioctl cmd
 *   arglen   The length of 'arg'
 *
 ****************************************************************************/

int udp_ioctl(FAR struct udp_conn_s *conn,
              int cmd, FAR void *arg, size_t arglen)
{
  int ret = OK;

  switch (cmd)
    {
      case FIONREAD:
        *(FAR int *)((uintptr_t)arg) =
          iob_get_queue_size(&conn->readahead);
        break;
      default:
        ret = -EINVAL;
        break;
    }

  return ret;
}
