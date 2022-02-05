/*
 * (C) 2022- TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sh_env_tag {
	void (*destroy)(struct sh_env_tag *sh_env_tag);
	bool (*add)(struct sh_env_tag *sh_env_tag, const char *name, const char *value);
	bool (*add1)(struct sh_env_tag *sh_env_tag, const char *namevalue);
	char *(*get)(struct sh_env_tag *sh_env_tag, int index, char **value);
	void *private_data;
} sh_env_t;

sh_env_t *create_sh_env(void);

typedef struct cfg_data_tag {
	char *username;
	char *term;					// ex. "ttermpro.exe %s %d"
	char *termopt;				// ex. "/KR-UTF8"
	char *shell;				// ex. "/usr/bin/bash"
	char *term_type;			// terminal type ex. "vt100"
	char *change_dir;			// cd した後、シェル起動
	int port_start;				// default lowest port number
	int port_range;				// default number of ports
	int cl_port;
	bool home_chdir;			// chdir to HOME
	bool enable_loginshell;		// login shell flag
	int telsock_timeout;		// telnet socket timeout
	bool enable_agent_proxy;	// ssh agent proxy
	bool dumb;
	bool debug_flag;			// debug mode
	bool (*save)(struct cfg_data_tag *cfg_data_tag, const char *fname);
	bool (*load)(struct cfg_data_tag *cfg_data_tag, const char *fname);
	void (*destroy)(struct cfg_data_tag *cfg_data_tag);
	void (*dump)(struct cfg_data_tag *cfg_data_tag, void (*print)(const char* msg, ...));
	sh_env_t *sh_env;
	void *private_data;
} cfg_data_t;

cfg_data_t *load_cfg(const char *cfg);
cfg_data_t *create_cfg(void);

#ifdef __cplusplus
}
#endif
