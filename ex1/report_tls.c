/*
 * Copyright (c) 2015 Bob Beck <beck@obtuse.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * A handy little utility function you can use when we talk about Certificate
 * details in Exercise 1b - No need to use this for the first part.
 *
 * Try calling this after the handshake is completed, and experiment with it
 *
 *
 * (Originally my work from OpenBSD netcat)
 */

#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <tls.h>
#include <unistd.h>

void
report_tls(struct tls * tls_ctx, char * host)
{
	time_t t;
	const char *ocsp_url;

	fprintf(stderr, "TLS handshake negotiated %s/%s with host %s\n",
	    tls_conn_version(tls_ctx), tls_conn_cipher(tls_ctx), host);
	fprintf(stderr, "Peer name: %s\n", host);
	if (tls_peer_cert_subject(tls_ctx))
		fprintf(stderr, "Subject: %s\n",
		    tls_peer_cert_subject(tls_ctx));
	if (tls_peer_cert_issuer(tls_ctx))
		fprintf(stderr, "Issuer: %s\n",
		    tls_peer_cert_issuer(tls_ctx));
	if ((t = tls_peer_cert_notbefore(tls_ctx)) != -1)
		fprintf(stderr, "Valid From: %s", ctime(&t));
	if ((t = tls_peer_cert_notafter(tls_ctx)) != -1)
		fprintf(stderr, "Valid Until: %s", ctime(&t));
	if (tls_peer_cert_hash(tls_ctx))
		fprintf(stderr, "Cert Hash: %s\n",
		    tls_peer_cert_hash(tls_ctx));
	ocsp_url = tls_peer_ocsp_url(tls_ctx);
	if (ocsp_url != NULL)
		fprintf(stderr, "OCSP URL: %s\n", ocsp_url);
	switch (tls_peer_ocsp_response_status(tls_ctx)) {
	case TLS_OCSP_RESPONSE_SUCCESSFUL:
		fprintf(stderr, "OCSP Stapling: %s\n",
		    tls_peer_ocsp_result(tls_ctx) == NULL ?  "" :
		    tls_peer_ocsp_result(tls_ctx));
		fprintf(stderr,
		    "  response_status=%d cert_status=%d crl_reason=%d\n",
		    tls_peer_ocsp_response_status(tls_ctx),
		    tls_peer_ocsp_cert_status(tls_ctx),
		    tls_peer_ocsp_crl_reason(tls_ctx));
		t = tls_peer_ocsp_this_update(tls_ctx);
		fprintf(stderr, "  this update: %s",
		    t != -1 ? ctime(&t) : "\n");
		t =  tls_peer_ocsp_next_update(tls_ctx);
		fprintf(stderr, "  next update: %s",
		    t != -1 ? ctime(&t) : "\n");
		t =  tls_peer_ocsp_revocation_time(tls_ctx);
		fprintf(stderr, "  revocation: %s",
		    t != -1 ? ctime(&t) : "\n");
		break;
	case -1:
		break;
	default:
		fprintf(stderr, "OCSP Stapling:  failure - response_status %d (%s)\n",
		    tls_peer_ocsp_response_status(tls_ctx),
		    tls_peer_ocsp_result(tls_ctx) == NULL ?  "" :
		    tls_peer_ocsp_result(tls_ctx));
		break;

	}
}
