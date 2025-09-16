/*
 * (C) 2022- TeraTerm Project
 * All rights reserved.
 *
 * This file is part of CygTerm+
 *
 * CygTerm+ is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License (GPL) as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * CygTerm+ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with cygterm; if not, see <https://www.gnu.org/licenses/>.
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
