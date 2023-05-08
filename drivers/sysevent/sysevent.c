/****************************************************************************
 * drivers/sysevent/sysevent.c
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

#include <debug.h>
#include <fcntl.h>
#include <sched.h>
#include <strings.h>
#include <sys/param.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>

#include <nuttx/kmalloc.h>
#include <nuttx/init.h>
#include <nuttx/sysevent/sysevent.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FORMAT_COMMA_SIZE 1     /* Size of , */
#define FORMAT_COLON_SIZE 1     /* Size of : */
#define FORMAT_QUOTES_SIZE 2    /* Size of "" */

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum data_type_e
{
  LONG_T = 0,                   /* Int type */
  STR_T                         /* String type */
};

/****************************************************************************
 * External Definitions
 ****************************************************************************/

extern int write_sysevent_kfifo(char *kbuf, int size);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int get_long_integer_size(long number)
{
  int num_count;
  long tmp;

  num_count = 0;
  if (number < 0)
    {
      num_count++;
    }

  tmp = number;
  do
    {
      num_count++;
      tmp = tmp / 10;
    }
  while (tmp != 0);
  return num_count;
}

/****************************************************************************
 * Name: event_format
 *
 * Description:
 *   Formate a event to json style format.
 *   EventId id -t time -paraList {"key":integar,"key_2":"string"}
 *
 ****************************************************************************/

static int event_format(struct sysevent_s *event, char *jsonstr)
{
  struct sysevent_payload_s *payload;

  sprintf(jsonstr, "EventId %d -t %lld -paraList ", event->eventid,
          event->ts.tv_sec);
  payload = event->head->next;
  if (payload == NULL)
    {
      strcat(jsonstr, "{}");
      return 0;
    }

  strcat(jsonstr, "{");
  while (payload != NULL)
    {
      strcat(jsonstr, "\"");
      strcat(jsonstr, payload->key);
      strcat(jsonstr, "\":");
      if (payload->type == STR_T)
        {
          strcat(jsonstr, "\"");
        }

      strcat(jsonstr, payload->value);
      if (payload->type == STR_T)
        {
          strcat(jsonstr, "\"");
        }

      payload = payload->next;
      if (payload != NULL)
        {
          strcat(jsonstr, ",");
        }
      else
        {
          strcat(jsonstr, "}");
        }
    }

  _info("jsonstr:%s", jsonstr);
  return 0;
}

static void free_list(struct sysevent_payload_s *head)
{
  struct sysevent_payload_s *p;
  struct sysevent_payload_s *q;
  p = head->next;
  while (p != NULL)
    {
      q = p->next;
      kmm_free(p);
      p = q;
    }

  head->next = NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sysevent_alloc
 *
 * Description:
 *   Allocate a sysevent struct.
 *
 ****************************************************************************/

struct sysevent_s *sysevent_alloc(unsigned int eventid)
{
  struct sysevent_s *event;

  event = kmm_zalloc(sizeof(struct sysevent_s));
  if (event == NULL)
    {
      _err("sysevent create error");
      goto err;
    }

  event->eventid = eventid;
  event->head = kmm_zalloc(sizeof(struct sysevent_payload_s));
  if (event->head == NULL)
    {
      _err("Fail to create sysevent->head");
      goto err_head_nomem;
    }

  event->head->next = NULL;
  if (OSINIT_HW_READY())
    {
      clock_gettime(CLOCK_REALTIME, &event->ts);
    }

  event->len = get_long_integer_size(event->eventid) +
               get_long_integer_size(event->ts.tv_sec);

  return event;

err_head_nomem:
  kmm_free(event);
err:
  return NULL;
}

/****************************************************************************
 * Name: sysevent_add_int
 *
 * Description:
 *   Add a integer type parameter to sysevent.
 *
 ****************************************************************************/

int sysevent_add_int(FAR struct sysevent_s *event, FAR const char *key,
                     long value)
{
  struct sysevent_payload_s *head;
  struct sysevent_payload_s *payload;
  int key_size;
  int value_size;
  int payload_size;

  if (event == NULL || key == NULL)
    {
      return -EPERM;
    }

  key_size = strlen(key);
  value_size = get_long_integer_size(value);
  payload_size = sizeof(struct sysevent_payload_s);
  payload = kmm_zalloc(payload_size + key_size + 1 + value_size + 1);
  if (payload == NULL)
    {
      _err("mievent_payload create error");
      return -ENOMEM;
    }

  payload->key = (char *)payload + payload_size;
  payload->value = (char *)payload + payload_size + key_size + 1;
  payload->type = LONG_T;
  payload->next = NULL;
  snprintf(payload->key, key_size + 1, "%s", key);
  snprintf(payload->value, value_size + 1, "%ld", value);

  head = event->head;
  payload->next = head->next;
  head->next = payload;
  event->len += key_size + FORMAT_QUOTES_SIZE + FORMAT_COLON_SIZE;
  event->len += value_size + FORMAT_COMMA_SIZE;
  return 0;
}

/****************************************************************************
 * Name: sysevent_add_str
 *
 * Description:
 *   Add a string type parameter to sysevent.
 *
 ****************************************************************************/

int sysevent_add_str(FAR struct sysevent_s *event, FAR const char *key,
                     const char *value)
{
  struct sysevent_payload_s *head;
  struct sysevent_payload_s *payload;
  int key_size;
  int value_size;
  int payload_size;

  if (event == NULL || key == NULL || value == NULL)
    {
      return -EPERM;
    }

  key_size = strlen(key);
  value_size = strlen(value);
  payload_size = sizeof(struct sysevent_payload_s);
  payload = kmm_zalloc(payload_size + key_size + 1 + value_size + 1);
  if (payload == NULL)
    {
      _err("mievent_payload create error");
      return -ENOMEM;
    }

  payload->key = (char *)payload + payload_size;
  payload->value = (char *)payload + payload_size + key_size + 1;
  payload->type = STR_T;
  payload->next = NULL;
  snprintf(payload->key, key_size + 1, "%s", key);
  snprintf(payload->value, value_size + 1, "%s", value);

  head = event->head;
  payload->next = head->next;
  head->next = payload;
  event->len += key_size + FORMAT_QUOTES_SIZE + FORMAT_COLON_SIZE;
  event->len += value_size + FORMAT_QUOTES_SIZE + FORMAT_COMMA_SIZE;
  return 0;
}

/****************************************************************************
 * Name: sysevent_write
 *
 * Description:
 *   Write a sysevent to sysevent device's kfifo. Kernel modules should call
 *   this function to report a sysevent.
 *
 ****************************************************************************/

int sysevent_write(struct sysevent_s *event)
{
  int head_size;
  int buffer_size;
  char *buffer;

  if (event == NULL)
    {
      return -EPERM;
    }

  head_size = strlen("EventId  -t  -paraList {}");
  buffer_size = head_size + event->len;
  buffer = kmm_zalloc(buffer_size);
  if (buffer == NULL)
    {
      _err("buffer create error");
      return -ENOMEM;
    }

  event_format(event, buffer);
  write_sysevent_kfifo(buffer, strlen(buffer));

  kmm_free(buffer);
  free_list(event->head);
  return 0;
}

/****************************************************************************
 * Name: sysevent_destroy
 *
 * Description:
 *   Destroy sysevent struct after write it to sysevent device.
 *
 ****************************************************************************/

void sysevent_destroy(struct sysevent_s *event)
{
  if (event != NULL)
    {
      free_list(event->head);
      kmm_free(event->head);
      kmm_free(event);
    }
}
