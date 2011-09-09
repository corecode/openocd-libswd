/*
 * $Id$
 *
 * SWD Transport Core Body File for OpenOCD.
 *
 * Copyright (C) 2011 Tomasz Boleslaw CEDRO
 * cederom@tlen.pl, http://www.tomek.cedro.info
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Tomasz Boleslaw CEDRO nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.*
 *
 * Written by Tomasz Boleslaw CEDRO <cederom@tlen.pl>, 2010-2011;
 *
 */

/** \file swd_core.c SWD Transport Core Body File for OpenOCD.
 * SWD Transport Layer creates bridge between target and the interface driver
 * functions. Target functions create high level operations on the device's
 * DAP (Debug Access Port), while interface driver passes electrical signals
 * in and out of the physical device. Transport is implemented using LibSWD,
 * and external open-source SWD framework.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <transport/swd.h>
#include <jtag/interface.h> //we want this here to use extern global *interface
#include <unistd.h>

///Unfortunalety OpenOCD use globals to pass information so we need to use it too.
extern struct jtag_interface *jtag_interface;

/** @{ swd_arm_adi_v5 Function set to support existing ARM ADI v5 target's
 * infrastructure.
 */

int oocd_swd_queue_idcode_read(struct adiv5_dap *dap, uint8_t *ack, uint32_t *data){
	int retval;
	retval=swd_dp_read_idcode(jtag_interface->transport->ctx, SWD_OPERATION_ENQUEUE, (int **)&data);
	if (retval<0) {
		LOG_ERROR("swd_dp_read_idcode() error: %s ", swd_error_string(retval));
		return ERROR_FAIL;
	} else return ERROR_OK; 
}

int oocd_swd_queue_dp_read(struct adiv5_dap *dap, unsigned reg, uint32_t *data){
	int retval;
	retval=swd_dp_read((swd_ctx_t *)jtag_interface->transport->ctx, SWD_OPERATION_ENQUEUE, reg, (int **)&data);
	if (retval<0){
		LOG_ERROR("swd_dp_read() error: %s ", swd_error_string(retval));
		return ERROR_FAIL;
	}
	return ERROR_OK;
}

int oocd_swd_queue_dp_write(struct adiv5_dap *dap, unsigned reg, uint32_t data){
	int retval;
	retval=swd_dp_write((swd_ctx_t *)jtag_interface->transport->ctx, SWD_OPERATION_ENQUEUE, (char) reg, (int *) &data);
	if (retval<0){
		LOG_ERROR("swd_dp_write() error: %s ", swd_error_string(retval));
		return ERROR_FAIL;
	}
	return ERROR_OK;
}

int oocd_swd_queue_ap_read(struct adiv5_dap *dap, unsigned reg, uint32_t *data){
	int retval;
	retval=swd_ap_read((swd_ctx_t *)jtag_interface->transport->ctx, SWD_OPERATION_ENQUEUE, (char) reg, (int **) &data);
	if (retval<0){
		LOG_ERROR("swd_ap_read() error: %s ", swd_error_string(retval));
		return ERROR_FAIL;
	}
	return ERROR_OK;
}

int oocd_swd_queue_ap_write(struct adiv5_dap *dap, unsigned reg, uint32_t data){
	int retval;
	retval=swd_ap_write((swd_ctx_t *)jtag_interface->transport->ctx, SWD_OPERATION_ENQUEUE, (char) reg, (int *) &data);
	if (retval<0){
		LOG_ERROR("swd_ap_write() error: %s ", swd_error_string(retval));
		return ERROR_FAIL;
	}
	return ERROR_OK;
}

int oocd_swd_queue_ap_abort(struct adiv5_dap *dap, uint8_t *ack){
	//int retval;
	//char reg=SWD_DP_ABORT_ADDR;
     LOG_ERROR("oocd_swd_queue_ap_abort() not yet implemented");
	return ERROR_FAIL;
}

int oocd_swd_run(struct adiv5_dap *dap){
	int retval;
	retval=swd_cmdq_flush((swd_ctx_t *)jtag_interface->transport->ctx, SWD_OPERATION_EXECUTE);
	if (retval<0){
		LOG_ERROR("swd_cmdq_flush() error: %s", swd_error_string(retval));
		return retval;
	} else return ERROR_OK;
}


// Transport select prepares selected transport for later use and bus/target initialization.
// TODO: We are operating on global interface pointer, change it into function parameter asap.
int oocd_swd_transport_init(struct command_context *ctx){
	LOG_DEBUG("entering function...");
	int retval, *idcode;

	//struct target *target = get_current_target(ctx);
	//struct arm *arm = target_to_arm(target);
	//struct adiv5_dap *dap = arm->dap;

	/**
	 * Initialize the driver to work with selected transport.
	 * Because we can work on existing context there is no need to destroy it,
	 * as it can be used on next try.
	 */
	retval=swd_dap_detect((swd_ctx_t *)jtag_interface->transport->ctx, SWD_OPERATION_EXECUTE, &idcode);
	if (retval<0) {
          LOG_ERROR("swd_dap_detect() error %d (%s)", retval, swd_error_string(retval));
          return retval;
     }

     LOG_INFO("SWD transport initialization complete. Found IDCODE=0x%08X.", *idcode);
	return ERROR_OK;
}

/**
 * Select SWD transport on interface pointed by global *jtag_interface structure.
 * Select is assumed to be called before transport init. It prepares everything,
 * including context memory and command set for higher layers, but not hardware
 * and does not interrogate target device (with IDCODE read that is done by
 * transport init call). This function does not touch the hardware because
 * hardware use signals that are not yet read from config file at this point!
 */
int oocd_swd_transport_select(struct command_context *ctx){
	LOG_DEBUG("entering function...");
	int retval;

     jtag_interface->transport=(struct transport *)&oocd_transport_swd;
	//struct target *target = get_current_target(ctx);
	// retval = register_commands(ctx, NULL, swd_handlers);

     // Create SWD_CTX if nesessary
	if (!jtag_interface->transport->ctx){
		/** Transport was not yet initialized. */
		jtag_interface->transport->ctx=swd_init();
		if (jtag_interface->transport->ctx==NULL) {
			LOG_ERROR("Cannot initialize SWD context!");
			return ERROR_FAIL;
		}
		LOG_INFO("New SWD context initialized at 0x%08X", (int)jtag_interface->transport->ctx);
	} else LOG_INFO("Working on existing transport context at 0x%0X...", (int)&jtag_interface->transport->ctx);

	retval=swd_log_level_inherit(jtag_interface->transport->ctx, debug_level);
	if (retval<0){
		LOG_ERROR("Unable to set log level: %s", swd_error_string(retval));
		return ERROR_FAIL;
	} 
     LOG_DEBUG("SWD Transport selection complete...");
	return ERROR_OK;
}


struct transport oocd_transport_swd = {
     .name = "swd",
     .select = oocd_swd_transport_select,
     .init = oocd_swd_transport_init,
     .ctx = NULL,
     .next = NULL,
};

const struct dap_ops oocd_dap_ops_swd = {
	.is_swd = true,

	.queue_idcode_read = oocd_swd_queue_idcode_read,
	.queue_dp_read = oocd_swd_queue_dp_read,
	.queue_dp_write = oocd_swd_queue_dp_write,
	.queue_ap_read = oocd_swd_queue_ap_read,
	.queue_ap_write = oocd_swd_queue_ap_write,
	.queue_ap_abort = oocd_swd_queue_ap_abort,
	.run = oocd_swd_run,
};




/** Register SWD Transport at program startup. */
static void swd_constructor(void) __attribute__((constructor));
static void swd_constructor(void)
{
             transport_register((struct transport *)&oocd_transport_swd);
}

/** Returns true if the current debug session
 * is using SWD as its transport.
 */
bool transport_is_swd(void)
{
	return get_current_transport() == &oocd_transport_swd;
}




///////////////////////////////////////////////////////////////////////////////
// BELOW UGLY FUNCTIONS TO MAKE OLD THINGS WORK AND COMPILE, REMOVE THEM ASAP
///////////////////////////////////////////////////////////////////////////////

#include <target/arm.h>

/*
 * This represents the bits which must be sent out on TMS/SWDIO to
 * switch a DAP implemented using an SWJ-DP module into SWD mode.
 * These bits are stored (and transmitted) LSB-first.
 *
 * See the DAP-Lite specification, section 2.2.5 for information
 * about making the debug link select SWD or JTAG.  (Similar info
 * is in a few other ARM documents.)
 */
static const uint8_t jtag2swd_bitseq[] = {
	/* More than 50 TCK/SWCLK cycles with TMS/SWDIO high,
	 * putting both JTAG and SWD logic into reset state.
	 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	/* Switching sequence enables SWD and disables JTAG
	 * NOTE: bits in the DP's IDCODE may expose the need for
	 * an old/obsolete/deprecated sequence (0xb6 0xed).
	 */
	0x9e, 0xe7,
	/* More than 50 TCK/SWCLK cycles with TMS/SWDIO high,
	 * putting both JTAG and SWD logic into reset state.
	 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};



/**
 * Put the debug link into SWD mode, if the target supports it.
 * The link's initial mode may be either JTAG (for example,
 * with SWJ-DP after reset) or SWD.
 *
 * @param target Enters SWD mode (if possible).
 *
 * Note that targets using the JTAG-DP do not support SWD, and that
 * some targets which could otherwise support it may have have been
 * configured to disable SWD signaling
 *
 * @return ERROR_OK or else a fault code.
 */
int dap_to_swd(struct target *target)
{
	LOG_INFO("dap_to_swd()");
	return ERROR_OK;
}

/** @} */
