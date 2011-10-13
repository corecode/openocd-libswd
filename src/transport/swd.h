/*
 * $Id$
 *
 * SWD Transport Header File for OpenOCD.
 *
 * Copyright (C) 2011 Tomasz Boleslaw CEDRO
 * cederom@tle.pl, http://www.tomek.cedro.info
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

/** \file swd.h SWD Transport Header File for OpenOCD. */

#ifndef OOCD_SWD_H
#define OOCD_SWD_H


#ifdef HAVE_CONFIG_H 
#include "config.h"
#endif

#include <libswd.h>
#include <target/arm.h>
#include <target/arm_adi_v5.h>
#include <helper/log.h>
#include <interface/interface.h>
#include <jtag/interface.h>

int oocd_swd_queue_idcode_read(struct adiv5_dap *dap, uint8_t *ack, uint32_t *data);
int oocd_swd_queue_dp_read(struct adiv5_dap *dap, unsigned reg, uint32_t *data);
int oocd_swd_queue_dp_write(struct adiv5_dap *dap, unsigned reg, uint32_t data);
int oocd_swd_queue_ap_read(struct adiv5_dap *dap, unsigned reg, uint32_t *data);
int oocd_swd_queue_ap_write(struct adiv5_dap *dap, unsigned reg, uint32_t data);
int oocd_swd_queue_ap_abort(struct adiv5_dap *dap, uint8_t *ack);
int oocd_swd_run(struct adiv5_dap *dap);
int oocd_swd_transport_init(struct command_context *ctx);
int oocd_swd_transport_select(struct command_context *ctx);

extern struct transport oocd_transport_swd;
extern const struct dap_ops oocd_dap_ops_swd;

int swd_register_commands(struct command_context *cmd_ctx);

#endif
