/*
 * Copyright (c) 2010 by David Brownell
 * Copyright (C) 2011 Tomasz Boleslaw CEDRO (http://www.tomek.cedro.info)
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

#ifndef OOCD_TRANSPORT_H
#define OOCD_TRANSPORT_H

#include "helper/command.h"
#include "transport/swd.h"

/**
 * Wrapper for transport lifecycle operations.
 *
 * OpenOCD talks to targets through some kind of debugging
 * or programming adapter, using some protocol that probably
 * has target-specific aspects.
 *
 * A "transport" reflects electrical protocol to the target,
 * e..g jtag, swd, spi, uart, ... NOT the messaging protocols
 * layered over it (e.g. JTAG has eICE, CoreSight, Nexus, OnCE,
 * and more).
 *
 * In addition to the lifecycle operations packaged by this
 * structure, a transport also involves  an interface supported
 * by debug adapters and used by components such as debug targets.
 * For non-debug transports,  there may be interfaces used to
 * write to flash chips.
 */
struct transport {
	/**
	 * Each transport has a unique name, used to select it
	 * from among the alternatives.  Examples might include
	 * "jtag", * "swd", "AVR_ISP" and more.
	 */
	const char *name;

	/**
	 * Each transport can have its own context to operate and store internals.
	 * Transport implementation/library can use it to store config, queue, etc.
	 * In more advanced modular configuration each component has its own
	 * context for internal representation and functional data exchange.
	 * Context is stored using *void type for more flexibility and
	 * architecture/library independent design. 
	 */
	void *ctx;

	/**
	 * Implements transport "select" command, activating the transport to be
	 * used in this debug session from among the set supported by the debug
	 * adapter being used.  When a transport is selected, this method registers
	 * its commands and activates the transport (ie. allocates memory,
	 * initializes external libraries, prepares queue, resets the link).
	 * Note that "select" only prepares transport for use, but does not
	 * operate on target as "init" does (ie. interrogates scan chain) and is
	 * called before "init". "Select" should be called only once per session.
	 *
	 * Note (TC@20110524): Should we allow multiple transport switching in future?
	 */
	int (*select)(struct command_context *ctx);

	/**
	 * Implements transport "init" used by server startup to validate transport
	 * configuration (ie. JTAG/SWD interrogates the scan chain against the list
	 * of expected devices) and must be called after transport is already
	 * "selected".
	 */
	int (*init)(struct command_context *ctx);

	/**
	 * Transports are stored in a singly linked list.
	 */
	struct transport *next;
};

typedef struct transport oocd_transport_t;

int transport_register(struct transport *new_transport);

struct transport *get_current_transport(void);

int transport_register_commands(struct command_context *ctx);

COMMAND_HELPER(transport_list_parse, char ***vector);

int allow_transports(struct command_context *ctx, const char **vector);

bool transports_are_declared(void);

extern const char *oocd_transport_jtag_only[];
extern const char *oocd_transport_swd_only[];



#endif
