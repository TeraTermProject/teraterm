/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2004- TeraTerm Project
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
// PuTTY is copyright 1997-2021 Simon Tatham.

/* 
 * ソース全体をそのままプロジェクトに追加すると、
 * 使わない部分で依存先が増え、さらにその依存先が増えたり競合が起きるので、
 * PuTTY のソースからコピーしてきたコード
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ssh.h"
#include "mpint.h"
#include "putty.h"
#include "network.h"
#include "sshcr.h"
#include "pageant.h"


// from pageant.c (PuTTY 0.76)
#define DECL_EXT_ENUM(id, name) id,
enum Extension { KNOWN_EXTENSIONS(DECL_EXT_ENUM) EXT_UNKNOWN };
#define DEF_EXT_NAMES(id, name) PTRLEN_DECL_LITERAL(name),
static const ptrlen extension_names[] = { KNOWN_EXTENSIONS(DEF_EXT_NAMES) };

// from pageant.c (PuTTY 0.76)
typedef struct PageantClientOp {
    strbuf *buf;
    bool request_made;
    BinarySink_DELEGATE_IMPLEMENTATION;
    BinarySource_IMPLEMENTATION;
} PageantClientOp;

// from pageant.c (PuTTY 0.76)
typedef struct KeyListEntry {
    ptrlen blob, comment;
    uint32_t flags;
} KeyListEntry;

// from pageant.c (PuTTY 0.76)
typedef struct KeyList {
    strbuf *raw_data;
    KeyListEntry *keys;
    size_t nkeys;
    bool broken;
} KeyList;

// from pageant.c (PuTTY 0.76)
static PageantClientOp *pageant_client_op_new(void)
{
    PageantClientOp *pco = snew(PageantClientOp);
    pco->buf = strbuf_new_for_agent_query();
    pco->request_made = false;
    BinarySink_DELEGATE_INIT(pco, pco->buf);
    BinarySource_INIT(pco, "", 0);
    return pco;
}

// from pageant.c (PuTTY 0.76)
static void pageant_client_op_free(PageantClientOp *pco)
{
    if (pco->buf)
        strbuf_free(pco->buf);
    sfree(pco);
}

// from pageant.c (PuTTY 0.76)
static unsigned pageant_client_op_query(PageantClientOp *pco)
{
    /* Since we use the same strbuf for the request and the response,
     * check by assertion that we aren't embarrassingly sending a
     * previous response back to the agent */
    assert(!pco->request_made);
    pco->request_made = true;

    void *response_raw;
    int resplen_raw;
    agent_query_synchronous(pco->buf, &response_raw, &resplen_raw);
    strbuf_clear(pco->buf);
    put_data(pco->buf, response_raw, resplen_raw);
    sfree(response_raw);

    /* The data coming back from agent_query_synchronous will have
        * its length field prepended. So we start by parsing it as an
        * SSH-formatted string, and then reinitialise our
        * BinarySource with the interior of that string. */
    BinarySource_INIT_PL(pco, ptrlen_from_strbuf(pco->buf));
    BinarySource_INIT_PL(pco, get_string(pco));

    /* Strip off and directly return the type byte, which every client
     * will need, to save a boilerplate get_byte at each call site */
    unsigned reply_type = get_byte(pco);
    if (get_err(pco))
        reply_type = 256;              /* out-of-range code */
    return reply_type;
}

// from pageant.c (PuTTY 0.76)
void keylist_free(KeyList *kl)
{
    sfree(kl->keys);
    strbuf_free(kl->raw_data);
    sfree(kl);
}

// from pageant.c (PuTTY 0.76)
static PageantClientOp *pageant_request_keylist_1(void)
{
    PageantClientOp *pco = pageant_client_op_new();
    put_byte(pco, SSH1_AGENTC_REQUEST_RSA_IDENTITIES);
    if (pageant_client_op_query(pco) == SSH1_AGENT_RSA_IDENTITIES_ANSWER)
        return pco;
    pageant_client_op_free(pco);
    return NULL;
}

// from pageant.c (PuTTY 0.76)
static PageantClientOp *pageant_request_keylist_2(void)
{
    PageantClientOp *pco = pageant_client_op_new();
    put_byte(pco, SSH2_AGENTC_REQUEST_IDENTITIES);
    if (pageant_client_op_query(pco) == SSH2_AGENT_IDENTITIES_ANSWER)
        return pco;
    pageant_client_op_free(pco);
    return NULL;
}

// from pageant.c (PuTTY 0.76)
static PageantClientOp *pageant_request_keylist_extended(void)
{
    PageantClientOp *pco = pageant_client_op_new();
    put_byte(pco, SSH2_AGENTC_EXTENSION);
    put_stringpl(pco, extension_names[EXT_LIST_EXTENDED]);
    if (pageant_client_op_query(pco) == SSH_AGENT_SUCCESS)
        return pco;
    pageant_client_op_free(pco);
    return NULL;
}

// from pageant.c (PuTTY 0.76)
KeyList *pageant_get_keylist(unsigned ssh_version)
{
    PageantClientOp *pco;
    bool list_is_extended = false;

    if (ssh_version == 1) {
        pco = pageant_request_keylist_1();
    } else {
        if ((pco = pageant_request_keylist_extended()) != NULL)
            list_is_extended = true;
        else
            pco = pageant_request_keylist_2();
    }

    if (!pco)
        return NULL;

    KeyList *kl = snew(KeyList);
    kl->nkeys = get_uint32(pco);
    kl->keys = snewn(kl->nkeys, struct KeyListEntry);
    kl->broken = false;

    for (size_t i = 0; i < kl->nkeys && !get_err(pco); i++) {
        if (ssh_version == 1) {
            int bloblen = rsa_ssh1_public_blob_len(
                make_ptrlen(get_ptr(pco), get_avail(pco)));
            if (bloblen < 0) {
                kl->broken = true;
                bloblen = 0;
            }
            kl->keys[i].blob = get_data(pco, bloblen);
        } else {
            kl->keys[i].blob = get_string(pco);
        }
        kl->keys[i].comment = get_string(pco);

        if (list_is_extended) {
            ptrlen key_ext_info = get_string(pco);
            BinarySource src[1];
            BinarySource_BARE_INIT_PL(src, key_ext_info);

            kl->keys[i].flags = get_uint32(src);
        } else {
            kl->keys[i].flags = 0;
        }
    }

    if (get_err(pco))
        kl->broken = true;
    kl->raw_data = pco->buf;
    pco->buf = NULL;
    pageant_client_op_free(pco);
    return kl;
}

// from sshrsa.c (PuTTY 0.76)
/* Given an SSH-1 public key blob, determine its length. */
int rsa_ssh1_public_blob_len(ptrlen data)
{
    BinarySource src[1];

    BinarySource_BARE_INIT_PL(src, data);

    /* Expect a length word, then exponent and modulus. (It doesn't
     * even matter which order.) */
    get_uint32(src);
    mp_free(get_mp_ssh1(src));
    mp_free(get_mp_ssh1(src));

    if (get_err(src))
        return -1;

    /* Return the number of bytes consumed. */
    return src->pos;
}

// from windows/winpgnt.c (PuTTY 0.76)
void noise_ultralight(NoiseSourceId id, unsigned long data)
{
    /* Pageant doesn't use random numbers, so we ignore this */
}

// from windows/window.c (PuTTY 0.76)
/*
 * Print a modal (Really Bad) message box and perform a fatal exit.
 */
void modalfatalbox(const char *fmt, ...)
{
    va_list ap;
    char *message, *title;

    va_start(ap, fmt);
    message = dupvprintf(fmt, ap);
    va_end(ap);
    title = dupprintf("%s Fatal Error", "TTSSH");
    MessageBox(NULL, message, title,
               MB_SYSTEMMODAL | MB_ICONERROR | MB_OK);
    sfree(message);
    sfree(title);
}


/*
agent_query_synchronous()@aqsync.c ... include

rsa_ssh1_public_blob_len()
	mp_free()@crypto\mpint.c ... include
		smemclr()@utils\smemclr.c ... include

putty_get_ssh1_keylen()@libputty.c
	rsa_ssh1_public_blob_len()@sshrsa.c ... import

strbuf_new()@utils\strbuf.c ... include

safemalloc()@memory.c
	out_of_memory()@utils\out_of_memory.c ... include
		modalfatalbox()@windows\window.c ... import

wm_copydata_agent_query()@windows\agent-client.c ... include
	got_advapi()@windows\utils\security.c ... include

agent_connect()@windows\agent-client.c
	new_named_pipe_client()@windows\named-pipe-client.c ... include
		connect_to_named_pipe()@windows\named-pipe-client.c
			win_strerror()@windows\utils\win_strerror.c ... include
		new_error_socket_consume_string()@errsock.c ... include
		make_handle_socket()@windows\handle-socket.c ... include
			bufchain_init()@utils\bufchain.c ... include
			handle_input_new()@windows\handle-io.c ... include
				ensure_ready_event_setup()@@windows\handle-io.c
					add_handle_wait()@windows\handle-wait.c
			handle_output_new()@windows\handle-io.c
			psb_init()@utils\log_proxy_stderr.c ... include
			handle_stderr()@windows\handle-socket.c
				log_proxy_stderr()@utils\log_proxy_stderr.c
			handle_sentdata()@windows\handle-socket.c
				plug_closing_system_error()@windows\network.c ... ?

agent_named_pipe_name()@windows\agent-client.c
	get_username()@windows\utils\get_username.c ... include
		load_system32_dll()@windows\utils\load_system32_dll.c ... include
			dupcat()@utils\dupcat.c ... include
			get_system_dir()@windows\utils\get_system_dir.c ... include
	capi_obfuscate_string()@windows\utils\cryptoapi.c ... include
		dupstr()@utils\dupstr.c
		ssh_sha256@crypto\sha256-select.c ... include

HandleSocket_sockvt@windows\handle-socket.c
	sk_handle_set_frozen()@windows\handle-socket.c
		queue_toplevel_callback()@callback.c ... include
	sk_handle_close()@windows\handle-socket.c
		delete_callbacks_for_context()@callback.c
			ssh_sha256_sw@ssh.h ... ?
		sk_addr_free()@windows\network.c ... ?

tree234@utils\tree234.c ... include

BinarySink_put_fmtv()@marshal.c
	burnstr()@utils\burnstr.c ... include
	dupprintf()@utils\dupprintf.c ... include

putty_get_version()@libputty.c
	ver@version.c ... include


if include sshrsa.c ...
rsa_components()@sshrsa.c
	key_components_new()@sshpubk.c
	key_components_add_text()@sshpubk.c
	key_components_add_mp()@sshpubk.c
ssh_rsakex_encrypt()@sshrsa.c
	hash_simple()@sshauxcrypt.c
ssh_rsakex_decrypt()@sshrsa.c
	hash_simple()@sshauxcrypt.c
rsa_ssh1_encrypt()@sshrsa.c
	random_read()@sshrand.c

if include sshauxcropt.c ...
mac_simple()@sshauxcropt.c coflicts mac_simple()@keyfiles-putty.c
*/
