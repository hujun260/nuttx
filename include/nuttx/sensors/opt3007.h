/****************************************************************************
 * include/nuttx/sensors/opt3007.h
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

#ifndef __INCLUDE_NUTTX_SENSORS_OPT3007_H
#define __INCLUDE_NUTTX_SENSORS_OPT3007_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/ioexpander/ioexpander.h>

#if defined CONFIG_SENSORS_OPT3007

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct opt3007_config_s
{
  uint8_t addr;                           /* I2C address */
  int freq;                               /* I2C frequency */
  FAR struct i2c_master_s *i2c;           /* I2C interface */
  FAR struct ioexpander_dev_s *ioedev;    /* Ioexpander device */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Inline Functions
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: opt3007_register
 *
 * Description:
 *   Register the OPT3007 character device as 'devno'
 *
 * Input Parameters:
 *   devno   - The user specifies device number, from 0.
 *   i2c     - An instance of the I2C interface to use to communicate with
 *              OPT3007
 *   config  - The board config function for the device.
 *
 * Returned Value:
 *   Return 0 if the driver was successfully initialize; A negated errno
 *   value is returned on any failure.
 *
 * Assumptions/Limitations:
 *   none
 *
 ****************************************************************************/

int opt3007_register(int devno, FAR const struct opt3007_config_s *config);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* CONFIG_SENSORS_OPT3007 */
#endif /* __INCLUDE_NUTTX_SENSORS_OPT3007_H */