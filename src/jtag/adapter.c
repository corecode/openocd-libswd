/***************************************************************************
 *   Copyright (C) 2005 by Dominic Rath                                    *
 *   Dominic.Rath@gmx.de                                                   *
 *                                                                         *
 *   Copyright (C) 2007-2010 Øyvind Harboe                                 *
 *   oyvind.harboe@zylin.com                                               *
 *                                                                         *
 *   Copyright (C) 2009 SoftPLC Corporation                                *
 *       http://softplc.com                                                *
 *   dick@softplc.com                                                      *
 *                                                                         *
 *   Copyright (C) 2009 Zachary T Welch                                    *
 *   zw@superlucidity.net                                                  *
 *                                                                         *
 *   Copyright (C) 2011 Tomasz Boleslaw CEDRO                              *
 *   cederom@tlen.pl, http://www.tomek.cedro.info                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "jtag.h"
#include "minidriver.h"
#include "interface.h"
#include "interfaces.h"
#include <transport/transport.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

/**
 * @file
 * Holds support for configuring debug adapters from TCl scripts.
 */

extern struct jtag_interface *jtag_interface;

static int
jim_adapter_name(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	Jim_GetOptInfo goi;
	Jim_GetOpt_Setup(&goi, interp, argc-1, argv + 1);

	/* return the name of the interface */
	/* TCL code might need to know the exact type... */
	/* FUTURE: we allow this as a means to "set" the interface. */
	if (goi.argc != 0) {
		Jim_WrongNumArgs(goi.interp, 1, goi.argv-1, "(no params)");
		return JIM_ERR;
	}
	const char *name = jtag_interface ? jtag_interface->name : NULL;
	Jim_SetResultString(goi.interp, name ? : "undefined", -1);
	return JIM_OK;
}


static int default_khz(int khz, int *jtag_speed)
{
	LOG_ERROR("Translation from khz to jtag_speed not implemented");
	return ERROR_FAIL;
}

static int default_speed_div(int speed, int *khz)
{
	LOG_ERROR("Translation from jtag_speed to khz not implemented");
	return ERROR_FAIL;
}

static int default_power_dropout(int *dropout)
{
	*dropout = 0; /* by default we can't detect power dropout */
	return ERROR_OK;
}

static int default_srst_asserted(int *srst_asserted)
{
	*srst_asserted = 0; /* by default we can't detect srst asserted */
	return ERROR_OK;
}

COMMAND_HANDLER(interface_transport_command)
{
	char **transports;
	int retval;

	retval = CALL_COMMAND_HANDLER(transport_list_parse, &transports);
	if (retval != ERROR_OK) {
		return retval;
	}

	retval = allow_transports(CMD_CTX, (const char **)transports);

	if (retval != ERROR_OK) {
		for (unsigned i = 0; transports[i]; i++)
			free(transports[i]);
		free(transports);
	}
	return retval;
}

COMMAND_HANDLER(handle_interface_list_command)
{
	if (strcmp(CMD_NAME, "interface_list") == 0 && CMD_ARGC > 0)
		return ERROR_COMMAND_SYNTAX_ERROR;

	command_print(CMD_CTX, "The following debug interfaces are available:");
	for (unsigned i = 0; NULL != jtag_interfaces[i]; i++)
	{
		const char *name = jtag_interfaces[i]->name;
		command_print(CMD_CTX, "%u: %s", i + 1, name);
	}

	return ERROR_OK;
}

COMMAND_HANDLER(handle_interface_command)
{
	int retval;

	/* check whether the interface is already configured */
	if (jtag_interface)
	{
		LOG_WARNING("Interface already configured, ignoring");
		return ERROR_OK;
	}

	/* interface name is a mandatory argument */
	if (CMD_ARGC != 1 || CMD_ARGV[0][0] == '\0')
		return ERROR_COMMAND_SYNTAX_ERROR;

	for (unsigned i = 0; NULL != jtag_interfaces[i]; i++)
	{
		if (strcmp(CMD_ARGV[0], jtag_interfaces[i]->name) != 0)
			continue;

		if (NULL != jtag_interfaces[i]->commands)
		{
			retval = register_commands(CMD_CTX, NULL,
					jtag_interfaces[i]->commands);
			if (ERROR_OK != retval)
				return retval;
		}

		jtag_interface = jtag_interfaces[i];

	/* LEGACY SUPPORT ... adapter drivers  must declare what
	 * transports they allow.  Until they all do so, assume
	 * the legacy drivers are JTAG-only
	 */
	if (!jtag_interface->transports)
		LOG_WARNING("Adapter driver '%s' did not declare "
			"which transports it allows; assuming "
			"legacy JTAG-only", jtag_interface->name);
		retval = allow_transports(CMD_CTX, jtag_interface->transports
						? jtag_interface->transports : oocd_transport_jtag_only);
			if (ERROR_OK != retval)
				return retval;

		if (jtag_interface->khz == NULL)
			jtag_interface->khz = default_khz;
		if (jtag_interface->speed_div == NULL)
			jtag_interface->speed_div = default_speed_div;
		if (jtag_interface->power_dropout == NULL)
			jtag_interface->power_dropout = default_power_dropout;
		if (jtag_interface->srst_asserted == NULL)
			jtag_interface->srst_asserted = default_srst_asserted;

		return ERROR_OK;
	}

	/* no valid interface was found (i.e. the configuration option,
	 * didn't match one of the compiled-in interfaces
	 */
	LOG_ERROR("The specified debug interface was not found (%s)",
				CMD_ARGV[0]);
	CALL_COMMAND_HANDLER(handle_interface_list_command);
	return ERROR_JTAG_INVALID_INTERFACE;
}

/** Interface signals handling routine that can add, delete and list signals.
 * Signal ADD requires signal name string and 32-bit mask, optionally a value.
 * Values are read as HEX. Signal DEL requires only signal name to delete.
 * Signal LIST will show marvelous table wits signal names, masks and values.
 * Interfaces should be defined in configuration file by TCL interface.
 * Parameters are checked before function execution.
 */
COMMAND_HANDLER(handle_interface_signal_command)
{
	LOG_DEBUG("entering function...");

	if (!jtag_interface){
		command_print(CMD_CTX, "You must initialize interface first!");
		return ERROR_FAIL;
	}

	if (CMD_ARGC<1 || CMD_ARGC>3){
		command_print(CMD_CTX, "Bad syntax!");
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	int sigmask;
	char signame[32];

	if (!strncasecmp(CMD_ARGV[0], "add", 3)){
		if (CMD_ARGC<3){
			LOG_ERROR("Usage: interface_signal add signal_name signal_mask");
			return ERROR_FAIL;
		}
		if (!strncpy(signame, CMD_ARGV[1], 32)){
			LOG_ERROR("Unable to copy signal name from parameter list!");
			return ERROR_FAIL;
		}
		// We are going to add interface signal.
		// Check the mask parameter.
		if (!sscanf(CMD_ARGV[2], "%x", &sigmask)){
			LOG_ERROR("Bad signal mask parameter! Use HEX value.");
			return ERROR_COMMAND_SYNTAX_ERROR;
		}
		// Now add the inetrface signal with specified parameters.
		return oocd_interface_signal_add(signame, sigmask);

	} else if (!strncasecmp(CMD_ARGV[0], "del", 3)){
		if (CMD_ARGC<2){
			LOG_ERROR("Usage: interface_signal del signal_name");
			return ERROR_FAIL;
		}
		// We are going to delete specified signal.
		return oocd_interface_signal_del((char *)CMD_ARGV[1]);

	} else if (!strncasecmp(CMD_ARGV[0], "list", 4)){
		//We are going to list available signals.
		oocd_interface_signal_t *sig;
		sig=jtag_interface->signal;
		command_print(CMD_CTX, "      Interface Signal Name      |    Mask    |   Value   ");
		command_print(CMD_CTX, "----------------------------------------------------------");
		while (sig) {
			command_print(CMD_CTX, "%32s | 0x%08X | 0x%08X", sig->name, sig->mask, sig->value);
			sig=sig->next;
		}
		// Also print warning if interface driver does not support bit-baning.
		if (!jtag_interface->bitbang) command_print(CMD_CTX, "WARNING: This interface does not support bit-baning!");
		return ERROR_OK;

	} else if (!strncasecmp(CMD_ARGV[0], "find", 4)){
		if (CMD_ARGC<2){
			LOG_ERROR("Usage: interface_signal find signal_name");
			return ERROR_FAIL;
		}
		// Find the signal and print its details.
		oocd_interface_signal_t *sig;
		if ( (sig=oocd_interface_signal_find((char *)CMD_ARGV[1])) != NULL ){
			command_print(CMD_CTX, "%s: mask=0x%08X value=0x%08X", sig->name, sig->mask, sig->value);
			return ERROR_OK;
		}
		// Or return information and error if not found.
		command_print(CMD_CTX, "Signal not found!");
		return ERROR_FAIL;
	}
	// For unknown parameter print error and return error code.
	command_print(CMD_CTX, "Unknown parameter!");
	return ERROR_COMMAND_SYNTAX_ERROR;
}

COMMAND_HANDLER(handle_reset_config_command)
{
	int new_cfg = 0;
	int mask = 0;

	/* Original versions cared about the order of these tokens:
	 *   reset_config signals [combination [trst_type [srst_type]]]
	 * They also clobbered the previous configuration even on error.
	 *
	 * Here we don't care about the order, and only change values
	 * which have been explicitly specified.
	 */
	for (; CMD_ARGC; CMD_ARGC--, CMD_ARGV++) {
		int tmp = 0;
		int m;

		/* gating */
		m = RESET_SRST_NO_GATING;
		if (strcmp(*CMD_ARGV, "srst_gates_jtag") == 0)
			/* default: don't use JTAG while SRST asserted */;
		else if (strcmp(*CMD_ARGV, "srst_nogate") == 0)
			tmp = RESET_SRST_NO_GATING;
		else
			m = 0;
		if (mask & m) {
			LOG_ERROR("extra reset_config %s spec (%s)",
					"gating", *CMD_ARGV);
			return ERROR_INVALID_ARGUMENTS;
		}
		if (m)
			goto next;

		/* signals */
		m = RESET_HAS_TRST | RESET_HAS_SRST;
		if (strcmp(*CMD_ARGV, "none") == 0)
			tmp = RESET_NONE;
		else if (strcmp(*CMD_ARGV, "trst_only") == 0)
			tmp = RESET_HAS_TRST;
		else if (strcmp(*CMD_ARGV, "srst_only") == 0)
			tmp = RESET_HAS_SRST;
		else if (strcmp(*CMD_ARGV, "trst_and_srst") == 0)
			tmp = RESET_HAS_TRST | RESET_HAS_SRST;
		else
			m = 0;
		if (mask & m) {
			LOG_ERROR("extra reset_config %s spec (%s)",
					"signal", *CMD_ARGV);
			return ERROR_INVALID_ARGUMENTS;
		}
		if (m)
			goto next;

		/* combination (options for broken wiring) */
		m = RESET_SRST_PULLS_TRST | RESET_TRST_PULLS_SRST;
		if (strcmp(*CMD_ARGV, "separate") == 0)
			/* separate reset lines - default */;
		else if (strcmp(*CMD_ARGV, "srst_pulls_trst") == 0)
			tmp |= RESET_SRST_PULLS_TRST;
		else if (strcmp(*CMD_ARGV, "trst_pulls_srst") == 0)
			tmp |= RESET_TRST_PULLS_SRST;
		else if (strcmp(*CMD_ARGV, "combined") == 0)
			tmp |= RESET_SRST_PULLS_TRST | RESET_TRST_PULLS_SRST;
		else
			m = 0;
		if (mask & m) {
			LOG_ERROR("extra reset_config %s spec (%s)",
					"combination", *CMD_ARGV);
			return ERROR_INVALID_ARGUMENTS;
		}
		if (m)
			goto next;

		/* trst_type (NOP without HAS_TRST) */
		m = RESET_TRST_OPEN_DRAIN;
		if (strcmp(*CMD_ARGV, "trst_open_drain") == 0)
			tmp |= RESET_TRST_OPEN_DRAIN;
		else if (strcmp(*CMD_ARGV, "trst_push_pull") == 0)
			/* push/pull from adapter - default */;
		else
			m = 0;
		if (mask & m) {
			LOG_ERROR("extra reset_config %s spec (%s)",
					"trst_type", *CMD_ARGV);
			return ERROR_INVALID_ARGUMENTS;
		}
		if (m)
			goto next;

		/* srst_type (NOP without HAS_SRST) */
		m |= RESET_SRST_PUSH_PULL;
		if (strcmp(*CMD_ARGV, "srst_push_pull") == 0)
			tmp |= RESET_SRST_PUSH_PULL;
		else if (strcmp(*CMD_ARGV, "srst_open_drain") == 0)
			/* open drain from adapter - default */;
		else
			m = 0;
		if (mask & m) {
			LOG_ERROR("extra reset_config %s spec (%s)",
					"srst_type", *CMD_ARGV);
			return ERROR_INVALID_ARGUMENTS;
		}
		if (m)
			goto next;

		/* caller provided nonsense; fail */
		LOG_ERROR("unknown reset_config flag (%s)", *CMD_ARGV);
		return ERROR_INVALID_ARGUMENTS;

next:
		/* Remember the bits which were specified (mask)
		 * and their new values (new_cfg).
		 */
		mask |= m;
		new_cfg |= tmp;
	}

	/* clear previous values of those bits, save new values */
	if (mask) {
		int old_cfg = jtag_get_reset_config();

		old_cfg &= ~mask;
		new_cfg |= old_cfg;
		jtag_set_reset_config(new_cfg);
	} else
		new_cfg = jtag_get_reset_config();


	/*
	 * Display the (now-)current reset mode
	 */
	char *modes[5];

	/* minimal JTAG has neither SRST nor TRST (so that's the default) */
	switch (new_cfg & (RESET_HAS_TRST | RESET_HAS_SRST)) {
	case RESET_HAS_SRST:
		modes[0] = "srst_only";
		break;
	case RESET_HAS_TRST:
		modes[0] = "trst_only";
		break;
	case RESET_TRST_AND_SRST:
		modes[0] = "trst_and_srst";
		break;
	default:
		modes[0] = "none";
		break;
	}

	/* normally SRST and TRST are decoupled; but bugs happen ... */
	switch (new_cfg & (RESET_SRST_PULLS_TRST | RESET_TRST_PULLS_SRST)) {
	case RESET_SRST_PULLS_TRST:
		modes[1] = "srst_pulls_trst";
		break;
	case RESET_TRST_PULLS_SRST:
		modes[1] = "trst_pulls_srst";
		break;
	case RESET_SRST_PULLS_TRST | RESET_TRST_PULLS_SRST:
		modes[1] = "combined";
		break;
	default:
		modes[1] = "separate";
		break;
	}

	/* TRST-less connectors include Altera, Xilinx, and minimal JTAG */
	if (new_cfg & RESET_HAS_TRST) {
		if (new_cfg & RESET_TRST_OPEN_DRAIN)
			modes[3] = " trst_open_drain";
		else
			modes[3] = " trst_push_pull";
	} else
		modes[3] = "";

	/* SRST-less connectors include TI-14, Xilinx, and minimal JTAG */
	if (new_cfg & RESET_HAS_SRST) {
		if (new_cfg & RESET_SRST_NO_GATING)
			modes[2] = " srst_nogate";
		else
			modes[2] = " srst_gates_jtag";

		if (new_cfg & RESET_SRST_PUSH_PULL)
			modes[4] = " srst_push_pull";
		else
			modes[4] = " srst_open_drain";
	} else {
		modes[2] = "";
		modes[4] = "";
	}

	command_print(CMD_CTX, "%s %s%s%s%s",
			modes[0], modes[1],
			modes[2], modes[3], modes[4]);

	return ERROR_OK;
}

COMMAND_HANDLER(handle_adapter_nsrst_delay_command)
{
	if (CMD_ARGC > 1)
		return ERROR_COMMAND_SYNTAX_ERROR;
	if (CMD_ARGC == 1)
	{
		unsigned delay;
		COMMAND_PARSE_NUMBER(uint, CMD_ARGV[0], delay);

		jtag_set_nsrst_delay(delay);
	}
	command_print(CMD_CTX, "adapter_nsrst_delay: %u", jtag_get_nsrst_delay());
	return ERROR_OK;
}

COMMAND_HANDLER(handle_adapter_nsrst_assert_width_command)
{
	if (CMD_ARGC > 1)
		return ERROR_COMMAND_SYNTAX_ERROR;
	if (CMD_ARGC == 1)
	{
		unsigned width;
		COMMAND_PARSE_NUMBER(uint, CMD_ARGV[0], width);

		jtag_set_nsrst_assert_width(width);
	}
	command_print(CMD_CTX, "adapter_nsrst_assert_width: %u", jtag_get_nsrst_assert_width());
	return ERROR_OK;
}



COMMAND_HANDLER(handle_adapter_khz_command)
{
	if (CMD_ARGC > 1)
		return ERROR_COMMAND_SYNTAX_ERROR;

	int retval = ERROR_OK;
	if (CMD_ARGC == 1)
	{
		unsigned khz = 0;
		COMMAND_PARSE_NUMBER(uint, CMD_ARGV[0], khz);

		retval = jtag_config_khz(khz);
		if (ERROR_OK != retval)
			return retval;
	}

	int cur_speed = jtag_get_speed_khz();
	retval = jtag_get_speed_readable(&cur_speed);
	if (ERROR_OK != retval)
		return retval;

	if (cur_speed)
		command_print(CMD_CTX, "%d kHz", cur_speed);
	else
		command_print(CMD_CTX, "RCLK - adaptive");

	return retval;
}

static const struct command_registration interface_command_handlers[] = {
	{
		.name = "adapter_khz",
		.handler = handle_adapter_khz_command,
		.mode = COMMAND_ANY,
		.help = "With an argument, change to the specified maximum "
			"jtag speed.  For JTAG, 0 KHz signifies adaptive "
			" clocking. "
			"With or without argument, display current setting.",
		.usage = "[khz]",
	},
	{
		.name = "adapter_name",
		.mode = COMMAND_ANY,
		.jim_handler = jim_adapter_name,
		.help = "Returns the name of the currently "
			"selected adapter (driver)",
	},
	{
		.name = "adapter_nsrst_delay",
		.handler = handle_adapter_nsrst_delay_command,
		.mode = COMMAND_ANY,
		.help = "delay after deasserting SRST in ms",
		.usage = "[milliseconds]",
	},
	{
		.name = "adapter_nsrst_assert_width",
		.handler = handle_adapter_nsrst_assert_width_command,
		.mode = COMMAND_ANY,
		.help = "delay after asserting SRST in ms",
		.usage = "[milliseconds]",
	},
	{
		.name = "interface",
		.handler = handle_interface_command,
		.mode = COMMAND_CONFIG,
		.help = "Select a debug adapter interface (driver)",
		.usage = "driver_name",
	},
	{
		.name = "interface_transports",
		.handler = interface_transport_command,
		.mode = COMMAND_CONFIG,
		.help = "Declare transports the interface supports.",
		.usage = "transport ... ",
	},
	{
		.name = "interface_list",
		.handler = handle_interface_list_command,
		.mode = COMMAND_ANY,
		.help = "List all built-in debug adapter interfaces (drivers)",
	},
	{
		.name = "interface_signal",
		.handler = handle_interface_signal_command,
		.mode = COMMAND_ANY,
		.help = "List, Find, Remove and Add interface signal mask",
		.usage = "(add|del|find|list) signal_name [mask]",
	},
	{
		.name = "reset_config",
		.handler = handle_reset_config_command,
		.mode = COMMAND_ANY,
		.help = "configure adapter reset behavior",
		.usage = "[none|trst_only|srst_only|trst_and_srst] "
			"[srst_pulls_trst|trst_pulls_srst|combined|separate] "
			"[srst_gates_jtag|srst_nogate] "
			"[trst_push_pull|trst_open_drain] "
			"[srst_push_pull|srst_open_drain]",
	},
	COMMAND_REGISTRATION_DONE
};

/**
 * Register the commands which deal with arbitrary debug adapter drivers.
 *
 * @todo Remove internal assumptions that all debug adapters use JTAG for
 * transport.  Various types and data structures are not named generically.
 */
int interface_register_commands(struct command_context *ctx)
{
	return register_commands(ctx, NULL, interface_command_handlers);
}
