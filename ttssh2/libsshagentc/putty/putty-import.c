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

// from windows/winmiscs.c (PuTTY 0.76)
/*
 * Windows implementation of smemclr (see misc.c) using SecureZeroMemory.
 */
void smemclr(void *b, size_t n) {
    if (b && n > 0)
        SecureZeroMemory(b, n);
}

// from be_misc.c (PuTTY 0.76)
void log_proxy_stderr(Plug *plug, ProxyStderrBuf *psb,
                      const void *vdata, size_t len)
{
    const char *data = (const char *)vdata;

    /*
     * This helper function allows us to collect the data written to a
     * local proxy command's standard error in whatever size chunks we
     * happen to get from its pipe, and whenever we have a complete
     * line, we pass it to plug_log.
     *
     * (We also do this when the buffer in psb fills up, to avoid just
     * allocating more and more memory forever, and also to keep Event
     * Log lines reasonably bounded in size.)
     *
     * Prerequisites: a plug to log to, and a ProxyStderrBuf stored
     * somewhere to collect any not-yet-output partial line.
     */

    while (len > 0) {
        /*
         * Copy as much data into psb->buf as will fit.
         */
        assert(psb->size < lenof(psb->buf));
        size_t to_consume = lenof(psb->buf) - psb->size;
        if (to_consume > len)
            to_consume = len;
        memcpy(psb->buf + psb->size, data, to_consume);
        data += to_consume;
        len -= to_consume;
        psb->size += to_consume;

        /*
         * Output any full lines in psb->buf.
         */
        size_t pos = 0;
        while (pos < psb->size) {
            char *nlpos = memchr(psb->buf + pos, '\n', psb->size - pos);
            if (!nlpos)
                break;

            /*
             * Found a newline in the buffer, so we can output a line.
             */
            size_t endpos = nlpos - psb->buf;
            while (endpos > pos && (psb->buf[endpos-1] == '\n' ||
                                    psb->buf[endpos-1] == '\r'))
                endpos--;
            char *msg = dupprintf(
                "proxy: %.*s", (int)(endpos - pos), psb->buf + pos);
            plug_log(plug, PLUGLOG_PROXY_MSG, NULL, 0, msg, 0);
            sfree(msg);

            pos = nlpos - psb->buf + 1;
            assert(pos <= psb->size);
        }

        /*
         * If the buffer is completely full and we didn't output
         * anything, then output the whole thing, flagging it as a
         * truncated line.
         */
        if (pos == 0 && psb->size == lenof(psb->buf)) {
            char *msg = dupprintf(
                "proxy (partial line): %.*s", (int)psb->size, psb->buf);
            plug_log(plug, PLUGLOG_PROXY_MSG, NULL, 0, msg, 0);
            sfree(msg);

            pos = psb->size = 0;
        }

        /*
         * Now move any remaining data up to the front of the buffer.
         */
        size_t newsize = psb->size - pos;
        if (newsize)
            memmove(psb->buf, psb->buf + pos, newsize);
        psb->size = newsize;

        /*
         * And loop round again if there's more data to be read from
         * our input.
         */
    }
}

// from be_misc.c (PuTTY 0.76)
void psb_init(ProxyStderrBuf *psb)
{
    psb->size = 0;
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

// from misc.c (PuTTY 0.76)
void out_of_memory(void)
{
    modalfatalbox("Out of memory");
}


/*
putty_get_ssh1_keylen()@libputty.c
	rsa_ssh1_public_blob_len()@sshrsa.c ... import

memory.c / mpint.c / utils.c
	smemclr()@windows/winmiscs.c ... import

safemalloc()@memory.c
	out_of_memory()@misc.c ... import
		modalfatalbox()@windows/window.c ... import

wm_copydata_agent_query()@windows\winpgnt.c
	got_advapi()@windows\winsecur.c ... include winsecur.c

agent_named_pipe_name()@windows\winpgnt.c
	get_username()@windows\winmisc.c ... include winmisc.c

agent_connect()@windows\winpgnt.c
	new_named_pipe_client()@winnpc.c ... include winnpc.c
		connect_to_named_pipe()@winnpc.c
		new_error_socket_consume_string()@errsock.c ... include errsock.c
		make_handle_socket()@winsocket.c ... include winsocket.c
			handle_input_new()@winhandl.c ... include winhandl.c
			handle_output_new()@winhandl.c
			psb_init()@be_misc.c ... import
			handle_stderr()@winhsock.c
				log_proxy_stderr()@be_misc.c ... import

agent_named_pipe_name()@windows\winpgnt.c
	capi_obfuscate_string()@windows\wincapi.c ... include wincapi.c
		ssh_sha256@sshsh256.c ... include sshsh256.c

handle_got_event()@windows\winhandl.c
	noise_ultralight()@windows/winpgnt.c

sk_handle_set_frozen()@windows\winhsock.c
	queue_toplevel_callback()@callback.c ... include callback.c
sk_handle_close()@windows\winhsock.c
	delete_callbacks_for_context()@callback.c

winmisc.c refers to tree234 ... include tree234.c



if include be_misc.c ...
backend_socket_log()@be_misc.c
	sk_getaddr()@windows\winnet.c
	sk_addr_needs_port()@windows\winnet.c
	conf_get_int()@conf.c
	logevent()@logging.c

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
