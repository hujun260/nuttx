/****************************************************************************
 * include/nuttx/sysevent/sysevent.h
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

#ifndef __INCLUDE_NUTTX_SYSEVENT_SYSEVENT_H
#define __INCLUDE_NUTTX_SYSEVENT_SYSEVENT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <sys/time.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct sysevent_payload_s
{
  int      type;                       /* Parameter type */
  FAR char *key;                       /* Parameter name */
  FAR char *value;                     /* Parameter value */
  FAR struct sysevent_payload_s *next; /* Point to next parameter struct */
};

struct sysevent_s
{
  unsigned int eventid;                /* Sysevent event id */
  unsigned int len;                    /* Sysevent len in json string format */
  struct timespec ts;                  /* Sysevent timestamp */
  FAR struct sysevent_payload_s *head; /* Sysevent parameter list */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: sysevent_alloc
 *
 * Description:
 *   Allocate a sysevent struct.
 *
 ****************************************************************************/

struct sysevent_s *sysevent_alloc(unsigned int eventid);

/****************************************************************************
 * Name: sysevent_add_int
 *
 * Description:
 *   Add a integer type parameter to sysevent.
 *
 ****************************************************************************/

int sysevent_add_int(FAR struct sysevent_s *event, FAR const char *key,
                     long value);

/****************************************************************************
 * Name: sysevent_add_str
 *
 * Description:
 *   Add a string type parameter to sysevent.
 *
 ****************************************************************************/

int sysevent_add_str(FAR struct sysevent_s *event, FAR const char *key,
                     const char *value);

/****************************************************************************
 * Name: sysevent_write
 *
 * Description:
 *   Write a sysevent to sysevent device's kfifo. Kernel modules should call
 *   this function to report a sysevent.
 *
 ****************************************************************************/

int sysevent_write(FAR struct sysevent_s *event);

/****************************************************************************
 * Name: sysevent_destroy
 *
 * Description:
 *   Destroy sysevent struct after write it to sysevent device.
 *
 ****************************************************************************/

void sysevent_destroy(FAR struct sysevent_s *event);

#endif  //__INCLUDE_NUTTX_SYSEVENT_SYSEVENT_H
