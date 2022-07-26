/****************************************************************************
 * drivers/power/pm_procfs.c
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

#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <debug.h>
#include <errno.h>
#include <dirent.h>

#include <nuttx/nuttx.h>
#include <nuttx/fs/dirent.h>
#include <nuttx/fs/fs.h>
#include <nuttx/fs/procfs.h>
#include <nuttx/kmalloc.h>
#include <nuttx/power/pm.h>

#include "pm.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STHDR "DOMAIN%d           WAKE         SLEEP         TOTAL\n"
#define STFMT "%-8s %8" PRIu32 "s %02" PRIu32 "%% %8" PRIu32 "s %02" \
              PRIu32 "%% %8" PRIu32 "s %02" PRIu32 "%%\n"

#define WAHDR "DOMAIN%d      STATE     COUNT      TIME\n"
#define WAFMT "%-12s %-10s %4" PRIu32 " %8" PRIu32 "s\n"

/* Determines the size of an intermediate buffer that must be large enough
 * to handle the longest line generated by this logic (plus a couple of
 * bytes).
 */

#define PM_LINELEN 128

#ifndef ARRAY_SIZE
#  define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

typedef ssize_t (*pm_read_t)(FAR struct file *filep,
                             FAR char *buffer, size_t buflen);

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* This structure describes one open "file" */

struct pm_file_s
{
  struct procfs_file_s base;  /* Base open file structure */
  char line[PM_LINELEN];      /* Pre-allocated buffer for formatted lines */
  int domain;                 /* Domain index */
  pm_read_t read;             /* Read function */
};

struct pm_file_ops_s
{
  FAR const char *name;
  pm_read_t read;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* File system methods */

static int     pm_open(FAR struct file *filep, FAR const char *relpath,
                       int oflags, mode_t mode);
static int     pm_close(FAR struct file *filep);
static ssize_t pm_read_state(FAR struct file *filep, FAR char *buffer,
                             size_t buflen);
static ssize_t pm_read_wakelock(FAR struct file *filep, FAR char *buffer,
                                size_t buflen);
static ssize_t pm_read(FAR struct file *filep, FAR char *buffer,
                       size_t buflen);
static int     pm_dup(FAR const struct file *oldp,
                      FAR struct file *newp);

static int     pm_opendir(FAR const char *relpath,
                          FAR struct fs_dirent_s *dir);
static int     pm_closedir(FAR struct fs_dirent_s *dir);
static int     pm_readdir(FAR struct fs_dirent_s *dir,
                          FAR struct dirent *entry);
static int     pm_rewinddir(FAR struct fs_dirent_s *dir);

static int     pm_stat(FAR const char *relpath, FAR struct stat *buf);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* See fs_mount.c -- this structure is explicitly extern'ed there.
 * We use the old-fashioned kind of initializers so that this will compile
 * with any compiler.
 */

const struct procfs_operations pm_operations =
{
  pm_open,       /* open */
  pm_close,      /* close */
  pm_read,       /* read */
  NULL,          /* write */

  pm_dup,        /* dup */

  pm_opendir,    /* opendir */
  pm_closedir,   /* closedir */
  pm_readdir,    /* readdir */
  pm_rewinddir,  /* rewinddir */

  pm_stat        /* stat */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct pm_file_ops_s g_pm_files[] =
{
  {"state",    pm_read_state},
  {"wakelock", pm_read_wakelock},
};

static FAR const char *g_pm_state[PM_COUNT] =
{
  "normal", "idle", "standby", "sleep"
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pm_open
 ****************************************************************************/

static int pm_open(FAR struct file *filep, FAR const char *relpath,
                   int oflags, mode_t mode)
{
  FAR struct pm_file_s *pmfile;
  int i;

  finfo("Open '%s'\n", relpath);

  /* This PROCFS file is read-only.  Any attempt to open with write access
   * is not permitted.
   */

  if ((oflags & O_WRONLY) != 0 || (oflags & O_RDONLY) == 0)
    {
      ferr("ERROR: Only O_RDONLY supported\n");
      return -EACCES;
    }

  /* Allocate a container to hold the file attributes */

  pmfile = (FAR struct pm_file_s *)kmm_zalloc(sizeof(struct pm_file_s));
  if (!pmfile)
    {
      ferr("ERROR: Failed to allocate file attributes\n");
      return -ENOMEM;
    }

  relpath += strlen("pm/");
  for (i = 0; i < ARRAY_SIZE(g_pm_files); i++)
    {
      if (strncmp(relpath, g_pm_files[i].name,
                  strlen(g_pm_files[i].name)) == 0)
        {
          pmfile->read = g_pm_files[i].read;
          break;
        }
    }

  pmfile->domain = atoi(relpath + strlen(g_pm_files[i].name));

  DEBUGASSERT(pmfile->read);
  DEBUGASSERT(pmfile->domain < CONFIG_PM_NDOMAINS);

  /* Save the attributes as the open-specific state in filep->f_priv */

  filep->f_priv = (FAR void *)pmfile;
  return OK;
}

/****************************************************************************
 * Name: pm_close
 ****************************************************************************/

static int pm_close(FAR struct file *filep)
{
  FAR struct pm_file_s *pmfile;

  /* Recover our private data from the struct file instance */

  pmfile = (FAR struct pm_file_s *)filep->f_priv;
  DEBUGASSERT(pmfile);

  /* Release the file attributes structure */

  kmm_free(pmfile);
  filep->f_priv = NULL;
  return OK;
}

static ssize_t pm_read_state(FAR struct file *filep, FAR char *buffer,
                             size_t buflen)
{
  FAR struct pm_domain_s *dom;
  FAR struct pm_file_s *pmfile;
  irqstate_t flags;
  size_t totalsize = 0;
  size_t linesize;
  size_t copysize;
  off_t offset;
  uint32_t sum = 0;
  uint32_t state;

  finfo("buffer=%p buflen=%d\n", buffer, (int)buflen);

  /* Recover our private data from the struct file instance */

  pmfile = (FAR struct pm_file_s *)filep->f_priv;
  dom    = &g_pmglobals.domain[pmfile->domain];
  DEBUGASSERT(pmfile);
  DEBUGASSERT(dom);

  /* Save the file offset and the user buffer information */

  offset = filep->f_pos;

  /* Then list the power state */

  linesize = snprintf(pmfile->line, PM_LINELEN, STHDR, pmfile->domain);
  copysize = procfs_memcpy(pmfile->line, linesize, buffer,
                           buflen, &offset);

  totalsize += copysize;

  flags = pm_lock(pmfile->domain);

  for (state = 0; state < PM_COUNT; state++)
    {
      sum += dom->wake[state].tv_sec + dom->sleep[state].tv_sec;
    }

  sum = sum ? sum : 1;

  for (state = 0; state < PM_COUNT && totalsize < buflen; state++)
    {
      time_t total;

      total = dom->wake[state].tv_sec + dom->sleep[state].tv_sec;

      linesize = snprintf(pmfile->line, PM_LINELEN, STFMT,
                          g_pm_state[state],
                          dom->wake[state].tv_sec,
                          100 * dom->wake[state].tv_sec / sum,
                          dom->sleep[state].tv_sec,
                          100 * dom->sleep[state].tv_sec / sum,
                          total,
                          100 * total / sum);
      buffer += copysize;
      buflen -= copysize;

      copysize = procfs_memcpy(pmfile->line, linesize, buffer,
                               buflen, &offset);

      totalsize += copysize;
    }

  pm_unlock(pmfile->domain, flags);

  filep->f_pos += totalsize;
  return totalsize;
}

static ssize_t pm_read_wakelock(FAR struct file *filep, FAR char *buffer,
                                size_t buflen)
{
  FAR struct pm_domain_s *dom;
  FAR struct pm_file_s *pmfile;
  FAR dq_entry_t *entry;
  irqstate_t flags;
  size_t totalsize = 0;
  size_t linesize;
  size_t copysize;
  off_t offset;

  finfo("buffer=%p buflen=%d\n", buffer, (int)buflen);

  /* Recover our private data from the struct file instance */

  pmfile = (FAR struct pm_file_s *)filep->f_priv;
  dom    = &g_pmglobals.domain[pmfile->domain];
  DEBUGASSERT(pmfile);
  DEBUGASSERT(dom);

  /* Save the file offset and the user buffer information */

  offset = filep->f_pos;

  /* Then list the power state */

  linesize = snprintf(pmfile->line, PM_LINELEN,
                      WAHDR, pmfile->domain);
  copysize = procfs_memcpy(pmfile->line, linesize, buffer,
                           buflen, &offset);

  totalsize += copysize;

  flags = pm_lock(pmfile->domain);

  entry = dq_peek(&dom->wakelockall);
  for (; entry && totalsize < buflen; entry = dq_next(entry))
    {
      FAR struct pm_wakelock_s *wakelock =
          container_of(entry, struct pm_wakelock_s, fsnode);
      time_t time = wakelock->elapse.tv_sec;

      buffer += copysize;
      buflen -= copysize;

      if (wakelock->count > 0)
        {
          struct timespec ts;

          clock_systime_timespec(&ts);
          clock_timespec_subtract(&ts, &wakelock->start, &ts);

          time += ts.tv_sec;
        }

      linesize = snprintf(pmfile->line, PM_LINELEN, WAFMT,
                          wakelock->name,
                          g_pm_state[wakelock->state],
                          wakelock->count,
                          time);

      copysize = procfs_memcpy(pmfile->line, linesize, buffer,
                               buflen, &offset);

      totalsize += copysize;
    }

  pm_unlock(pmfile->domain, flags);

  filep->f_pos += totalsize;
  return totalsize;
}

/****************************************************************************
 * Name: pm_read
 ****************************************************************************/

static ssize_t pm_read(FAR struct file *filep, FAR char *buffer,
                       size_t buflen)
{
  FAR struct pm_file_s *pmfile;

  pmfile = (FAR struct pm_file_s *)filep->f_priv;

  return pmfile->read(filep, buffer, buflen);
}

/****************************************************************************
 * Name: pm_dup
 *
 * Description:
 *   Duplicate open file data in the new file structure.
 *
 ****************************************************************************/

static int pm_dup(FAR const struct file *oldp, FAR struct file *newp)
{
  FAR struct pm_file_s *oldattr;
  FAR struct pm_file_s *newattr;

  finfo("Dup %p->%p\n", oldp, newp);

  /* Recover our private data from the old struct file instance */

  oldattr = (FAR struct pm_file_s *)oldp->f_priv;
  DEBUGASSERT(oldattr);

  /* Allocate a new container to hold the task and attribute selection */

  newattr = (FAR struct pm_file_s *)kmm_malloc(sizeof(struct pm_file_s));
  if (!newattr)
    {
      ferr("ERROR: Failed to allocate file attributes\n");
      return -ENOMEM;
    }

  /* The copy the file attributes from the old attributes to the new */

  memcpy(newattr, oldattr, sizeof(struct pm_file_s));

  /* Save the new attributes in the new file structure */

  newp->f_priv = (FAR void *)newattr;
  return OK;
}

/****************************************************************************
 * Name: pm_opendir
 *
 * Description:
 *   Open a directory for read access
 *
 ****************************************************************************/

static int pm_opendir(FAR const char *relpath, FAR struct fs_dirent_s *dir)
{
  FAR struct procfs_dir_priv_s *level1;

  finfo("relpath: \"%s\"\n", relpath ? relpath : "NULL");
  DEBUGASSERT(relpath && dir && !dir->u.procfs);

  /* Assume that path refers to the 1st level subdirectory.  Allocate the
   * level1 the dirent structure before checking.
   */

  level1 = kmm_zalloc(sizeof(struct procfs_dir_priv_s));
  if (level1 == NULL)
    {
      ferr("ERROR: Failed to allocate the level1 directory structure\n");
      return -ENOMEM;
    }

  /* Initialize base structure components */

  level1->level    = 1;
  level1->nentries = CONFIG_PM_NDOMAINS * ARRAY_SIZE(g_pm_files);

  dir->u.procfs = (FAR void *)level1;
  return OK;
}

/****************************************************************************
 * Name: pm_closedir
 *
 * Description: Close the directory listing
 *
 ****************************************************************************/

static int pm_closedir(FAR struct fs_dirent_s *dir)
{
  FAR struct procfs_dir_priv_s *level1;

  DEBUGASSERT(dir && dir->u.procfs);
  level1 = dir->u.procfs;

  kmm_free(level1);

  dir->u.procfs = NULL;
  return OK;
}

/****************************************************************************
 * Name: pm_readdir
 *
 * Description: Read the next directory entry
 *
 ****************************************************************************/

static int pm_readdir(FAR struct fs_dirent_s *dir,
                      FAR struct dirent *entry)
{
  FAR struct procfs_dir_priv_s *level1;
  int index;
  int domain;
  int fpos;

  DEBUGASSERT(dir && dir->u.procfs);
  level1 = dir->u.procfs;

  index = level1->index;
  if (index >= level1->nentries)
    {
      /* We signal the end of the directory by returning the special
       * error -ENOENT
       */

      finfo("Entry %d: End of directory\n", index);
      return -ENOENT;
    }

  domain = index / ARRAY_SIZE(g_pm_files);
  fpos   = index % ARRAY_SIZE(g_pm_files);

  entry->d_type = DTYPE_FILE;
  snprintf(entry->d_name, NAME_MAX + 1, "%s%d",
           g_pm_files[fpos].name, domain);

  level1->index++;
  return OK;
}

/****************************************************************************
 * Name: pm_rewindir
 *
 * Description: Reset directory read to the first entry
 *
 ****************************************************************************/

static int pm_rewinddir(FAR struct fs_dirent_s *dir)
{
  FAR struct procfs_dir_priv_s *level1;

  DEBUGASSERT(dir && dir->u.procfs);
  level1 = dir->u.procfs;

  level1->index = 0;
  return OK;
}

/****************************************************************************
 * Name: pm_stat
 *
 * Description: Return information about a file or directory
 *
 ****************************************************************************/

static int pm_stat(FAR const char *relpath, FAR struct stat *buf)
{
  memset(buf, 0, sizeof(struct stat));

  if (strcmp(relpath, "pm") == 0 || strcmp(relpath, "pm/") == 0)
    {
      buf->st_mode = S_IFDIR | S_IROTH | S_IRGRP | S_IRUSR;
    }
  else
    {
      buf->st_mode = S_IFREG | S_IROTH | S_IRGRP | S_IRUSR;
    }

  return OK;
}
