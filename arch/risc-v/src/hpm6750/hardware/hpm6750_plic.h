/****************************************************************************
 * arch/risc-v/src/hpm6750/hardware/hpm6750_plic.h
 *
 * SPDX-License-Identifier: Apache-2.0
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

#ifndef __ARCH_RISCV_SRC_HPM6750_HARDWARE_HPM6750_PLIC_H
#define __ARCH_RISCV_SRC_HPM6750_HARDWARE_HPM6750_PLIC_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define HPM6750_PLIC_PRIORITY    (HPM6750_PLIC_BASE + 0x000000)
#define HPM6750_PLIC_PENDING0    (HPM6750_PLIC_BASE + 0x001000)
#define HPM6750_PLIC_PENDING1    (HPM6750_PLIC_BASE + 0x001004)
#define HPM6750_PLIC_PENDING2    (HPM6750_PLIC_BASE + 0x001008)
#define HPM6750_PLIC_PENDING3    (HPM6750_PLIC_BASE + 0x00100C)
#define HPM6750_PLIC_INTEN0      (HPM6750_PLIC_BASE + 0x002000)
#define HPM6750_PLIC_INTEN1      (HPM6750_PLIC_BASE + 0x002004)
#define HPM6750_PLIC_INTEN2      (HPM6750_PLIC_BASE + 0x002008)
#define HPM6750_PLIC_INTEN3      (HPM6750_PLIC_BASE + 0x00200C)
#define HPM6750_PLIC_THRESHOLD   (HPM6750_PLIC_BASE + 0x200000)
#define HPM6750_PLIC_CLAIM       (HPM6750_PLIC_BASE + 0x200004)

#endif /* __ARCH_RISCV_SRC_HPM6750_HARDWARE_HPM6750_PLIC_H */
