/*
 * Copyright (c) 2011 Tomasz Boleslaw CEDRO <cederom@tlen.pl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/** @file Simple framework to bitbang the interface pins.
 */

#include <helper/command.h>
#include <jtag/interface.h>
#include <interface/interface.h>
#include "bitbang.h"

extern struct jtag_interface *jtag_interface;

/*-----------------------------------------------------------------------*/

/*
 * Infrastructure for Tcl interface to bitbang.
 */

/** Bitbang framework allows operation on interface signals, operating on HEX
 * values, to read and set their state. When only signal name is given then
 * read operation is performed and value returned for given port mask defined
 * by signal mask. Additional '=' parameter with following value with set this
 * value to the port (again restricted by the signal mask). Therefore it is
 * possible to set/get more than one bit/pin at once each with its own value.
 * In addition label 'hi'/'set' equals value 0xFFFFFFFF, 'lo'/'clr' equals 0.
 * Also note that read/write operation will affect selected port bits direction
 * as reading values will switch masked bits to input while writing will make
 * them outputs. This is also good way to change port behavior for some pins,
 * but beware that reading output may not always bring expected results as it
 * may impact device connected to that pin (i.e. Hi-Z input gives 1 undriven).
 * This functionality brings new possibilities to script additional features.
 */
COMMAND_HANDLER(handle_bitbang_command)
{
	LOG_DEBUG("%s", __func__);

	if (!jtag_interface){
		LOG_ERROR("You need to setup interface first!");
		return ERROR_FAIL;
	}

	if (!jtag_interface->bitbang){
		LOG_ERROR("This interface does not support bit-banging!");
		return ERROR_FAIL;
	}

	if (CMD_ARGC < 1) {
		LOG_ERROR("Bad syntax!");
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	if (jtag_interface->signal==NULL){
		LOG_ERROR("No signals defined, see 'interface_signal' command for help.");
		return ERROR_FAIL;
	}

	static oocd_interface_signal_t *sig;
	static unsigned int pn;
	static int retval;
	static char *mark, *signame, *sigval, pcmd[45];

	// Iterate through list of command parameters
	for (pn=0;pn<CMD_ARGC;pn++){
		// Make a local copy of parameter command to work on
		if (!strncpy(pcmd, CMD_ARGV[pn], 45)){
			LOG_ERROR("Cannot copy parameter: %s", CMD_ARGV[pn]);
			return ERROR_FAIL;
		}
		// Look for '=' mark to see if read or write will happen
		mark=strnstr(pcmd, "=", 45);
		if (!mark){
			// If no '=' was found then we read the signal value
			// Check if specified signal exists
			if (!(sig=oocd_interface_signal_find(pcmd))){
				LOG_ERROR("Unknown signal specified!");
				return ERROR_FAIL;
			}
			// Call the driver routine to do the actual signal read
			retval=jtag_interface->bitbang(NULL, pcmd, 1, (int *)&sig->value);
			if (retval!=ERROR_OK){
				LOG_ERROR("Unable to read signal: %s", pcmd);
				return ERROR_FAIL;
			} 
		} else {
			// If '=' was found then we write specified value to the specified signal
			// Get and verify the signal name first
			signame=strtok(pcmd, "=");
			sig=oocd_interface_signal_find(signame);
			if (!sig){
				LOG_ERROR("Unknown signal specified!");
				return ERROR_FAIL;
			}
			// Then read the HEX value or hi (all bits one) / lo (all bits zero)
			if (!(sigval=strtok(NULL, "="))){
				LOG_ERROR("No value specified! Use: hi, set, lo, clr, or HEX port value.");
				return ERROR_COMMAND_SYNTAX_ERROR;
			}
			if (!strncmp(sigval, "hi", 2) || !strncmp(sigval, "set", 3)){
				sig->value=0xffffffff&sig->mask;
			} else if (!strncmp(sigval, "lo", 2) || !strncmp(sigval, "clr", 3)){
				sig->value=0;
			} else if (!sscanf(sigval, "%x", (unsigned int *)&sig->value)){
				LOG_ERROR("Bad value parameter specified (can be: hi, set, lo, clr, or direct HEX port value)");
				return ERROR_COMMAND_SYNTAX_ERROR;
			}
			retval=jtag_interface->bitbang(NULL, signame, 0, (int *)&sig->value);
			if (retval!=ERROR_OK){
				LOG_ERROR("Unable to write signal: %s", signame);
				return ERROR_FAIL;
			}
		}
		command_print(CMD_CTX, "%s=0x%08X", sig->name, sig->value);
	}
	return ERROR_OK;
}

static const struct command_registration bitbang_commands[] = {
	{
		.name = "bitbang",
		.handler = handle_bitbang_command,
		.mode = COMMAND_ANY,
		.help =  "Perform bit-bang operations on interface signals (mask!).",
		.usage = "'signal_name' to read, 'signal_name'='port_hex_value' to write.",
	},
	COMMAND_REGISTRATION_DONE
};

int bitbang_register_commands(struct command_context *ctx)
{
	return register_commands(ctx, NULL, bitbang_commands);
}
