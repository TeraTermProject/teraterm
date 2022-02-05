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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "cygterm_cfg.h"

#define DUMP_ENABLE	1

// additional env vars given to a shell
//-------------------------------------

typedef struct sh_env_data_t {
	struct sh_env_data_t* next;
	char *name;
	char *value;
} sh_env_data_t;

typedef struct sh_env_private_tag {
	sh_env_data_t *envp;
} sh_env_private_t;

static bool env_add(sh_env_t *envp, const char* name, const char* value)
{
	sh_env_private_t *pr_data = (sh_env_private_t *)envp->private_data;
	sh_env_data_t* e;

	if (name[0] == 0) {
		return true;
	}
	e = (sh_env_data_t*)malloc(sizeof(*e));
	if (e == NULL) {
		return false;
	}

	e->name = strdup(name);
	e->value = strdup(value);
	e->next = NULL;

	if (pr_data->envp == NULL) {
		pr_data->envp = e;
		return true;
	}
	sh_env_data_t* env_data = pr_data->envp;
	sh_env_data_t* prev_env = NULL;
	while(1) {
		if (strcmp(env_data->name, name) == 0) {
			// “¯‚¶–¼‘O -> “ü‚ê‘Ö‚¦
			if (prev_env == NULL) {
				pr_data->envp = e;
			} else {
				prev_env->next = e;
				e->next = env_data->next;
			}
			free(env_data->name);
			free(env_data->value);
			free(env_data);
			break;
		}
		if (env_data->next == NULL) {
			// ÅŒã‚Ü‚Å—ˆ‚½
			env_data->next = e;
			break;
		}
		prev_env = env_data;
		env_data = env_data->next;
	}

	return true;
}

static bool env_add1(sh_env_t *envp, const char* name_value)
{
	if (name_value[0] == 0) {
		return true;
	}
	char *name = strdup(name_value);
	char *p = strchr(name, '=');
	char *value;
	if (p == NULL) {
		value = NULL;
	}
	else {
		*p = 0;
		value = strdup(p+1);
	}
	bool r = env_add(envp, name, value);
	free(value);
	free(name);
	return r;
}

static char *env_get(sh_env_t *envp, int index, char **value)
{
	sh_env_private_t *pr_data = (sh_env_private_t *)envp->private_data;
	sh_env_data_t* e = pr_data->envp;
	if (e == NULL) {
		return NULL;
	}
	while(1) {
		if (index == 0) {
			*value = e->value;
			return e->name;;
		}
		if (e->next == NULL) {
			*value = NULL;
			return NULL;
		}
		index--;
		e = e->next;
	}
}

static void env_destry_all(sh_env_t *envp)
{
	sh_env_private_t *pr_data = (sh_env_private_t *)envp->private_data;
	sh_env_data_t* e = pr_data->envp;
	if (e == NULL) {
		return;
	}
	pr_data->envp = NULL;
	while(1) {
		sh_env_data_t* e_next = e->next;
		e->next = NULL;
		free(e);
		e = e_next;
		if (e == NULL) {
			break;
		}
	}
}

static void env_destry(sh_env_t *envp)
{
	sh_env_private_t *pr_data = (sh_env_private_t *)envp->private_data;
	env_destry_all(envp);
	free(pr_data);
	envp->private_data = NULL;
	free(envp);
}

sh_env_t *create_sh_env(void)
{
	sh_env_t *sh_env = (sh_env_t *)calloc(sizeof(*sh_env), 1);
	if (sh_env == NULL) {
		return NULL;
	}
	sh_env_private_t *pr_data = (sh_env_private_t *)calloc(sizeof(*pr_data), 1);
	if (pr_data == NULL) {
		free(sh_env);
		return NULL;
	}
	sh_env->private_data = pr_data;
	sh_env->destroy = env_destry;
	sh_env->add = env_add;
	sh_env->add1 = env_add1;
	sh_env->get = env_get;

	return sh_env;
}

static bool is_bool_string(const char *s)
{
	if (strchr("YyTt", *s) != NULL)
		return true;
	if (atoi(s) > 0)
		return true;

	return false;
}

//==================================//
// parse line in configuration file //
//----------------------------------//
static void parse_cfg_line(char *buf, cfg_data_t *cfg)
{
	// "KEY = VALUE" format in each line.
	// skip leading/trailing blanks. KEY is not case-sensitive.
	char* p1;
	for (p1 = buf; isspace(*p1); ++p1);
	if (!isalpha(*p1)) {
		return; // comment line with non-alphabet 1st char
	}
	char* name = p1;
	for (++p1; isalnum(*p1) || *p1 == '_'; ++p1);
	char* p2;
	for (p2 = p1; isspace(*p2); ++p2);
	if (*p2 != '=') {
		return; // igonore line without '='
	}
	for (++p2; isspace(*p2); ++p2);
	char* val = p2;
	for (p2 += strlen(p2); isspace(*(p2-1)); --p2);
	*p1 = *p2 = 0;

	if (!strcasecmp(name, "TERM")) {
		// terminal emulator command line (host:%s, port#:%d)
		free(cfg->term);
		cfg->term = strdup(val);
	}
	else if (!strcasecmp(name, "SHELL")) {
		// shell command line
		if (strcasecmp(val, "AUTO") != 0) {
			free(cfg->shell);
			cfg->shell = strdup(val);
		}
	}
	else if (!strcasecmp(name, "PORT_START")) {
		// minimum port# for TELNET
		cfg->port_start = atoi(val);
	}
	else if (!strcasecmp(name, "PORT_RANGE")) {
		// number of ports for TELNET
		cfg->port_range = atoi(val);
	}
	else if (!strcasecmp(name, "TERM_TYPE")) {
		// terminal type name (maybe overridden by TELNET negotiation.)
		free(cfg->term_type);
		cfg->term_type = strdup(val);
	}
	else if (!strncasecmp(name, "ENV_", 4)) {
		// additional env vars given to a shell
		sh_env_t *sh_env = cfg->sh_env;
		sh_env->add1(sh_env, val);
	}
	else if (!strcasecmp(name, "HOME_CHDIR")) {
		// change directory to home
		if (is_bool_string(val)) {
			cfg->home_chdir = true;
		}
	}
	else if (!strcasecmp(name, "LOGIN_SHELL")) {
		// execute a shell as a login shell
		if (is_bool_string(val)) {
			cfg->enable_loginshell = true;
		}
	}
	else if (!strcasecmp(name, "SOCKET_TIMEOUT")) {
		// telnet socket timeout
		cfg->telsock_timeout = atoi(val);
	}
	else if (!strcasecmp(name, "SSH_AGENT_PROXY")) {
		// ssh-agent proxy
		if (is_bool_string(val)) {
			cfg->enable_agent_proxy = true;
		}
	}
	else if (!strcasecmp(name, "DEBUG")) {
		// debug mode
		if (is_bool_string(val)) {
			cfg->debug_flag = true;
		}
	}
}

typedef struct {
	sh_env_data_t *env;
} cfg_private_data_t;

static void destroy(cfg_data_t *cfg_data)
{
//	env_destry_all(cfg_data);
	sh_env_t *sh_env = cfg_data->sh_env;
	sh_env->destroy(sh_env);
	cfg_private_data_t *pr_data = (cfg_private_data_t *)cfg_data->private_data;
	free(pr_data);
	free(cfg_data);
}

// read each setting parameter
// configuration file (.cfg) path
static bool load_cfg(cfg_data_t *cfg_data, const char *conf)
{
	FILE* fp = fopen(conf, "r");
	if (fp == NULL) {
		return false;
	}

	char buf[BUFSIZ];
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		parse_cfg_line(buf, cfg_data);
	}
	fclose(fp);

	return true;
}

#if !defined(offsetof)
#define offsetof(s,m) ((size_t)&(((s*)0)->m))
#endif

#if DUMP_ENABLE
static void dump(cfg_data_t *cfg_data, void (*print)(const char* msg, ...))
{
	const static struct {
		const char *name;
		size_t offset;
		char type;
	} list[] = {
		{ "username", offsetof(cfg_data_t, username), 's' },
		{ "term", offsetof(cfg_data_t, term), 's' },
		{ "termopt", offsetof(cfg_data_t, termopt), 's' },
		{ "shell", offsetof(cfg_data_t, shell), 's' },
		{ "term_type", offsetof(cfg_data_t, term_type), 's' },
		{ "change_dir", offsetof(cfg_data_t, change_dir), 's' },
		{ "port_start", offsetof(cfg_data_t, port_start), 'i' },
		{ "port_range", offsetof(cfg_data_t, port_range), 'i' },
		{ "cl_port", offsetof(cfg_data_t, cl_port), 'i' },
		{ "home_chdir", offsetof(cfg_data_t, home_chdir), 'b' },
		{ "enable_loginshell", offsetof(cfg_data_t, enable_loginshell), 'b' },
		{ "telsock_timeout", offsetof(cfg_data_t, telsock_timeout), 'i' },
		{ "enable_agent_proxy", offsetof(cfg_data_t, enable_agent_proxy), 'b' },
		{ "dumb", offsetof(cfg_data_t, dumb), 'b' },
		{ "debug_flag", offsetof(cfg_data_t, debug_flag), 'b' },
	};
	for (int i = 0; i < (int)(sizeof(list)/sizeof(list[0])); i++) {
		uint8_t *p = (uint8_t *)cfg_data + list[i].offset;
		switch (list[i].type) {
		case 's': {
			char *str = *(char **)p;
			print("%s=%s", list[i].name, str == NULL ? "NULL" : str);
			break;
		}
		case 'i': {
			int i2 = *(int *)p;
			print("%s=%d(0x%x)", list[i].name, i2, i2);
			break;
		}
		case 'b': {
			bool b = *(bool *)p;
			print("%s=%i(%s)", list[i].name, b, b ? "true" : "false");
			break;
		}
		default:
			print("?");
			break;
		}
	}

	sh_env_t *sh_env = cfg_data->sh_env;
	for(int i = 0;;i++) {
		char *value;
		const char *env = sh_env->get(sh_env, i, &value);
		if (env == NULL) {
			break;
		}
		print("env %d %s=%s", i, env, value);
	}
}
#else
static void dump(cfg_data_t *cfg_data, void (*print)(const char* msg, ...))
{
	(void)cfg_data;
	(void)print;
}
#endif

cfg_data_t *create_cfg(void)
{
	cfg_data_t *cfg_data = (cfg_data_t *)calloc(sizeof(*cfg_data), 1);
	if (cfg_data == NULL) {
		return NULL;
	}
	cfg_private_data_t *pr_data = (cfg_private_data_t *)calloc(sizeof(*pr_data), 1);
	if (pr_data == NULL) {
		free(cfg_data);
		return NULL;
	}

	// data, func
	cfg_data->private_data = pr_data;
	cfg_data->sh_env = create_sh_env();
	cfg_data->destroy = destroy;
	cfg_data->load = load_cfg;
	cfg_data->dump = dump;

	return cfg_data;
}
