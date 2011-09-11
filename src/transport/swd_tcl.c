/*
 * $Id$
 *
 * TCL Interface for SWD Transport.
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

/** \file swd_tcl.c TCL Interface for SWD Transport.
 * This file contains TCL interface and functions to work with SWD transport.
 */

#include <transport/swd.h>

COMMAND_HANDLER(handle_swd_loglevel)
{
	int loglevel;
	struct target *target = get_current_target(CMD_CTX);
	struct arm *arm = target_to_arm(target);
	swd_ctx_t *swdctx = (swd_ctx_t *)arm->dap->ctx;

	switch (CMD_ARGC) {
	case 0:
		LOG_USER("Current SWD LogLevel[%d..%d] is: %d (%s)", SWD_LOGLEVEL_MIN, SWD_LOGLEVEL_MAX, swdctx->config.loglevel, swd_log_level_string(swdctx->config.loglevel));
		break;
	case 1:
		// We want to allow inherit current OpenOCD's debuglevel.
		if (strncasecmp(CMD_ARGV[0], "inherit", 7)==0) {
			if((loglevel=swd_log_level_inherit(swdctx, debug_level))<0){
				LOG_ERROR("LogLevel inherit failed!");
				return ERROR_FAIL;
			} else {
				LOG_USER("Using OpenOCD settings, SWD LogLevel[%d..%d] set to: %d (%s)", SWD_LOGLEVEL_MIN, SWD_LOGLEVEL_MAX, loglevel, swd_log_level_string(loglevel));
				return ERROR_OK; 
			}
		}
		// Or we want to set log level for SWD transport by hand.
		loglevel=atoi(CMD_ARGV[0]);
		if (loglevel<SWD_LOGLEVEL_MIN || loglevel>SWD_LOGLEVEL_MAX) {
			LOG_ERROR("Bad SWD LogLevel value!");
			return ERROR_FAIL;
		} else LOG_USER("Setting SWD LogLevel[%d..%d] to: %d (%s)", SWD_LOGLEVEL_MIN, SWD_LOGLEVEL_MAX, loglevel, swd_log_level_string(loglevel));
		if(swd_log_level_set(swdctx, loglevel)<0){
			return ERROR_FAIL;
		} else return ERROR_OK;
	}
	LOG_INFO("Available values:");
	for (int i=0;i<=SWD_LOGLEVEL_MAX;i++) LOG_INFO(" %d (%s)", i, swd_log_level_string(i));
	return ERROR_OK;
}

static const struct command_registration swd_subcommand_handlers[] = {
	{
		/*
		 * Set up SWD and JTAG targets identically, unless/until
		 * infrastructure improves ...  meanwhile, ignore all
		 * JTAG-specific stuff like IR length for SWD.
		 *
		 * REVISIT can we verify "just one SWD DAP" here/early?
		 */
		.name = "newtap",
		.jim_handler = jim_jtag_newtap,
		.mode = COMMAND_CONFIG,
		.help = "declare a new SWD DAP"
	},
	{
		.name = "loglevel",
		.handler = handle_swd_loglevel,
		.mode = COMMAND_ANY,
		.help = "set/inherit/get loglevel for SWD transport.",
	},

	COMMAND_REGISTRATION_DONE
};

static const struct command_registration swd_command_handlers[] = {
	{
		.name = "swd",
		.mode = COMMAND_ANY,
		.help = "SWD command group",
		.chain = swd_subcommand_handlers,
	},
	COMMAND_REGISTRATION_DONE
};

int swd_register_commands(struct command_context *cmd_ctx)
{
	return register_commands(cmd_ctx, NULL, swd_command_handlers);
}

/** @} */
