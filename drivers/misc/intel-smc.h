/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2017-2018, Intel Corporation
 */

#ifndef __INTEL_SMC_H
#define __INTEL_SMC_H

#include <linux/arm-smccc.h>
#include <linux/bitops.h>

/*
 * This file defines the Secure Monitor Call (SMC) message protocol used for
 * service layer driver in normal world (EL1) to communicate with secure
 * monitor software in Secure Monitor Exception Level 3 (EL3).
 *
 * This file is shared with secure firmware (FW) which is out of kernel tree.
 *
 * An ARM SMC instruction takes a function identifier and up to 6 64-bit
 * register values as arguments, and can return up to 4 64-bit register
 * value. The operation of the secure monitor is determined by the parameter
 * values passed in through registers.

 * EL1 and EL3 communicates pointer as physical address rather than the
 * virtual address.
 */

/*
 * Functions specified by ARM SMC Calling convention:
 *
 * FAST call executes atomic operations, returns when the requested operation
 * has completed.
 * STD call starts a operation which can be preempted by a non-secure
 * interrupt. The call can return before the requested operation has
 * completed.
 *
 * a0..a7 is used as register names in the descriptions below, on arm32
 * that translates to r0..r7 and on arm64 to w0..w7.
 */

#define INTEL_SIP_SMC_STD_CALL_VAL(func_num) \
	ARM_SMCCC_CALL_VAL(ARM_SMCCC_STD_CALL, ARM_SMCCC_SMC_64, \
	ARM_SMCCC_OWNER_SIP, (func_num))

#define INTEL_SIP_SMC_FAST_CALL_VAL(func_num) \
	ARM_SMCCC_CALL_VAL(ARM_SMCCC_FAST_CALL, ARM_SMCCC_SMC_64, \
	ARM_SMCCC_OWNER_SIP, (func_num))

/*
 * Return values in INTEL_SIP_SMC_* call
 *
 * INTEL_SIP_SMC_RETURN_UNKNOWN_FUNCTION:
 * Secure monitor software doesn't recognize the request.
 *
 * INTEL_SIP_SMC_STATUS_OK:
 * FPGA configuration completed successfully,
 * In case of FPGA configuration write operation, it means secure monitor
 * software can accept the next chunk of FPGA configuration data.
 *
 * INTEL_SIP_SMC_FPGA_CONFIG_STATUS_BUSY:
 * In case of FPGA configuration write operation, it means secure monitor
 * software is still processing previous data & can't accept the next chunk
 * of data. Service driver needs to issue
 * INTEL_SIP_SMC_FPGA_CONFIG_COMPLETED_WRITE call to query the
 * completed block(s).
 *
 * INTEL_SIP_SMC_FPGA_CONFIG_STATUS_ERROR:
 * There is error during the FPGA configuration process.
 */
#define INTEL_SIP_SMC_RETURN_UNKNOWN_FUNCTION		0xFFFFFFFF
#define INTEL_SIP_SMC_STATUS_OK				0x0
#define INTEL_SIP_SMC_FPGA_CONFIG_STATUS_BUSY		0x1
#define INTEL_SIP_SMC_FPGA_CONFIG_STATUS_REJECTED       0x2
#define INTEL_SIP_SMC_FPGA_CONFIG_STATUS_ERROR		0x4

/*
 * Request INTEL_SIP_SMC_FPGA_CONFIG_START
 *
 * Sync call used by service driver at EL1 to request the FPGA in EL3 to
 * be prepare to receive a new configuration.
 *
 * Call register usage:
 * a0: INTEL_SIP_SMC_FPGA_CONFIG_START.
 * a1: flag for full or partial configuration
 *    0 full reconfiguration.
 *    1 partial reconfiguration.
 * a2-7: not used.
 *
 * Return status:
 * a0: INTEL_SIP_SMC_STATUS_OK, or INTEL_SIP_SMC_FPGA_CONFIG_STATUS_ERROR.
 * a1-3: not used.
 */
#define INTEL_SIP_SMC_FUNCID_FPGA_CONFIG_START 1
#define INTEL_SIP_SMC_FPGA_CONFIG_START \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FPGA_CONFIG_START)

/*
 * Request INTEL_SIP_SMC_FPGA_CONFIG_WRITE
 *
 * Async call used by service driver at EL1 to provide FPGA configuration data
 * to secure world.
 *
 * Call register usage:
 * a0: INTEL_SIP_SMC_FPGA_CONFIG_WRITE.
 * a1: 64bit physical address of the configuration data memory block
 * a2: Size of configuration data block.
 * a3-7: not used.
 *
 * Return status:
 * a0: INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_FPGA_CONFIG_STATUS_BUSY or
 * INTEL_SIP_SMC_FPGA_CONFIG_STATUS_ERROR.
 * a1: 64bit physical address of 1st completed memory block if any completed
 * block, otherwise zero value.
 * a2: 64bit physical address of 2nd completed memory block if any completed
 * block, otherwise zero value.
 * a3: 64bit physical address of 3rd completed memory block if any completed
 * block, otherwise zero value.
 */
#define INTEL_SIP_SMC_FUNCID_FPGA_CONFIG_WRITE 2
#define INTEL_SIP_SMC_FPGA_CONFIG_WRITE \
	INTEL_SIP_SMC_STD_CALL_VAL(INTEL_SIP_SMC_FUNCID_FPGA_CONFIG_WRITE)

/*
 * Request INTEL_SIP_SMC_FPGA_CONFIG_COMPLETED_WRITE
 *
 * Sync call used by service driver at EL1 to track the completed write
 * transactions. This request is called after INTEL_SIP_SMC_FPGA_CONFIG_WRITE
 * call returns INTEL_SIP_SMC_FPGA_CONFIG_STATUS_BUSY.
 *
 * Call register usage:
 * a0: INTEL_SIP_SMC_FPGA_CONFIG_COMPLETED_WRITE.
 * a1-7: not used.
 *
 * Return status:
 * a0: INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_FPGA_CONFIG_STATUS_BUSY or
 * INTEL_SIP_SMC_FPGA_CONFIG_STATUS_ERROR.
 * a1: 64bit physical address of 1st completed memory block.
 * a2: 64bit physical address of 2nd completed memory block if
 * any completed block, otherwise zero value.
 * a3: 64bit physical address of 3rd completed memory block if
 * any completed block, otherwise zero value.
 */
#define INTEL_SIP_SMC_FUNCID_FPGA_CONFIG_COMPLETED_WRITE 3
#define INTEL_SIP_SMC_FPGA_CONFIG_COMPLETED_WRITE \
INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FPGA_CONFIG_COMPLETED_WRITE)

/*
 * Request INTEL_SIP_SMC_FPGA_CONFIG_ISDONE
 *
 * Sync call used by service driver at EL1 to inform secure world that all
 * data are sent, to check whether or not the secure world had completed
 * the FPGA configuration process.
 *
 * Call register usage:
 * a0: INTEL_SIP_SMC_FPGA_CONFIG_ISDONE.
 * a1-7: not used.
 *
 * Return status:
 * a0: INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_FPGA_CONFIG_STATUS_BUSY or
 * INTEL_SIP_SMC_FPGA_CONFIG_STATUS_ERROR.
 * a1-3: not used.
 */
#define INTEL_SIP_SMC_FUNCID_FPGA_CONFIG_ISDONE 4
#define INTEL_SIP_SMC_FPGA_CONFIG_ISDONE \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FPGA_CONFIG_ISDONE)

/*
 * Request INTEL_SIP_SMC_FPGA_CONFIG_GET_MEM
 *
 * Sync call used by service driver at EL1 to query the physical address of
 * memory block reserved by secure monitor software.
 *
 * Call register usage:
 * a0:INTEL_SIP_SMC_FPGA_CONFIG_GET_MEM.
 * a1-7: not used.
 *
 * Return status:
 * a0: INTEL_SIP_SMC_STATUS_OK or INTEL_SIP_SMC_FPGA_CONFIG_STATUS_ERROR.
 * a1: start of physical address of reserved memory block.
 * a2: size of reserved memory block.
 * a3: not used.
 */
#define INTEL_SIP_SMC_FUNCID_FPGA_CONFIG_GET_MEM 5
#define INTEL_SIP_SMC_FPGA_CONFIG_GET_MEM \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FPGA_CONFIG_GET_MEM)

/*
 * Request INTEL_SIP_SMC_FPGA_CONFIG_LOOPBACK
 *
 * For SMC loop-back mode only, used for internal integration, debugging
 * or troubleshooting.
 *
 * Call register usage:
 * a0: INTEL_SIP_SMC_FPGA_CONFIG_LOOPBACK.
 * a1-7: not used.
 *
 * Return status:
 * a0: INTEL_SIP_SMC_STATUS_OK or INTEL_SIP_SMC_FPGA_CONFIG_STATUS_ERROR.
 * a1-3: not used.
 */
#define INTEL_SIP_SMC_FUNCID_FPGA_CONFIG_LOOPBACK 6
#define INTEL_SIP_SMC_FPGA_CONFIG_LOOPBACK \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FPGA_CONFIG_LOOPBACK)

#endif
