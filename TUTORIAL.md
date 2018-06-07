
# Happy Bob's Libtls tutorial

libtls is shipped as part of libressl with OpenBSD. It is designed to be simpler to use than
other C based tls interfaces (especially native OpenSSL) to do "normal" things with TLS in programs.

### So what's libtls good for?

- Any C program that needs to use TLS with client or server side certificate verification using common methods

What is libtls not good for?

- Arbitrary crypto operations.
- Certificate mangling or generation
- Being your own CA
- Implementing Kerberos
- Using a different interface to sockets to talk to sockets.
- etc etc.

In a nutshell, libtls is designed to do the common things you do for making TLS connections easy. It is not designed to be a Swiss army knife of everything possible.

### Goals for this tutorial

This tutorial is designed for people with some C experience on a POSIX, BSD like machine with the latest libtls installed. It focuses on changes that are necessary to make an existing program written in C that uses the POSIX sockets api to use TLS over those same connections.

So if you go through this tutorial, what I hope you get out of it is:

- A basic review of sockets in C, with read and write and synchronous IO. Do this in [ex0](ex0)

- How to convert a basic client and server program to use tls instead of cleartext in [ex1](ex1)

- How to convert a more advanced client and server using asynchronous IO and poll() in [ex2](ex2)

- Enough libtls knowledge to know where to find more, and extend this into your own work.

# Setup and Review

First of all, to make use of this you want to clone this repository from github. Everything
we talk about here, and the exercises, use the code contained in this repository.

### First review basic TCP sockets in C, and read and write

Once you have cloned this repository, Have a look at and review the sample in

- [Exercise 0](ex0)

Get this example to compile and run on your machine, and make sure you can connect
to the server with the client. Review the basics in there for how sockets are set up
and connected. You will later on be modifying this program to use TLS.

### TLS uses X509 certificates

The side effect of this is that in order for us to do anything useful with TLS, we need the ability to make certificates to work with. Recall from above that libtls does *NOT* implement functionally for being your own certificate authority. Sadly this means, we need some common ground here.

The [CA](CA) directory is set up to generate test certificates using the [openssl](https://man.openbsd.org/openssl.1) command to implement a basic CA to sign certificates.

- Run "make" in the [CA](CA) directory to generate some test certificates.
- Make sure you can see the contents of the generated certificates using "openssl x509 -text -in"

# Basic libtls use

libtls programs use a few opaque structures:

- *struct tls_config* : a tls configuration, used in turn to configure tls contexts. Configuration includes things like what certificate and key to use as well as validation options. 
- *struct tls* : a tls ctx, all the context for a tls connection, be it a client or server. 

### libtls setup. 

Typical initialization for a program will initially set up the configuration:

- Optionally call [tls_init](https://man.openbsd.org/tls_init.3) to initialize the library
- Call [tls_config_new](https://man.openbsd.org/tls_config_new.3) to set up a new tls configuration
- Call [tls_config_set_ca_file](https://man.openbsd.org/tls_config_set_ca_file.3) to add your roots certs
- Optionally Call [tls_config_set_cert_file](https://man.openbsd.org/tls_config_set_cert_file.3) to add your own certificate - A server will normally do this. Clients may not if they are connecting without client authentication.
- Optionally Call [tls_config_set_key_file](https://man.openbsd.org/tls_config_set__key_file.3) to add your certificate key - A server will normally do this. Clients may not if they are connecting without client authentication.

Once this is done you have a configuration set up to potentially initiate or receive tls connections. to make use of that configuration you need to

- Get yourself a tls context using either
  - [tls_server](https://man.openbsd.org/tls_server.3) to set up a server context
  - [tls_client](https://man.openbsd.org/tls_client.3) to set up a client context

Once you have this you apply a configuration to a context using

- [tls_configure](https://man.openbsd.org/tls_configure.3) to take your server or client context, and apply the configuration to it.

Now you're actually ready make TLS connections.

### Making a connection and doing the TLS handshake

If you have a program that already uses TCP sockets, you have a client that connects using [connect](https://man.openbsd.org/connect.2) and a server that uses [accept](https://man.openbsd.org/accept.2) to make or accept connections.

In a client:
- after you call [connect](https://man.openbsd.org/connect.2), you call [tls_connect_socket](https://man.openbsd.org/tls_connect_socket.3) to associate a tls context to your connected socket

In a server:
- after you call [accept](https://man.openbsd.org/accept.2), you call [tls_accept_socket](https://man.openbsd.org/tls_accept_socket.3) to associate a tls context to your accepted socket

Finally you may *Optionally* call

- [tls_handshake](https://man.openbsd.org/tls_handshake.3) to complete the TLS handshake. This is only necessary to do if you wish to ensure the TLS handshake completes, and possibly examine results using some more advanced features. If you don't call this, the handshake will be completed automatically for you on the next step.

### Reading and writing from a TLS connection

Sending and receiving of data is done with [tls_read](https://man.openbsd.org/tls_read.3) and [tls_write](https://man.openbsd.org/tls_write.3). They are designed to be similar in use, and familiar to programmers that have experience with the normal POSIX [read](https://man.openbsd.org/read.2) and [write](https://man.openbsd.org/write.2) system calls. *HOWEVER* it is important to remember that they are not actually system calls, and behave subtly differently in some important ways.

tls_read and tls_write (and tls_handshake):

- Actually have a really good [man page](https://man.openbsd.org/tls_read.3) that gives details on how to use them in a number of situations.
  - Successful reads and writes may only write part of the data. You get told how much was read or written, just like with read and write (unlike OpenSSL)
- For the synchronous IO case, the typical use pattern is straightforward, and involves repeating the command in a loop as long as it returns TLS_WANT_POLLIN or TLS_WANT_POLLOUT.
- -1 return indicates a failure.


### Finishing with a TLS connection

Finally you should call

- [tls_close](https://man.openbsd.org/tls_close.3) on a tls context when it is finished. this does not close the underlying file descriptor, so you keep your old code to close the underlying socket when it is done. 

and with that you have enough to do [Exercise 1a](ex1). Stop after the first part, and we'll
continue below.

# Certificate information and Client Validation

## Client Validation

A conventional web browser style TLS connection involves and anonymous client, connecting to a server that identifies itself with a certificate. Client validation can be used by the server to require the client to present and validate a certificate.

- [tls_config_verify_client](https://man.openbsd.org/tls_config_verify_client.3) when used on a server will require the client to present a valid certificate.

- [tls_config_verify_client_optional](https://man.openbsd.org/tls_config_verify_client_optional.3) when used on a server will validate a client certificate if one is presented, but allow the client to connect anonymously

## Further certificate inspection

Libtls provides a number of functions to retrieve information from the certificate and session
after the handshake completes.

The basics include:

- [tls_conn_version](http://man.openbsd.org/tls_conn_version.3)
- [tls_conn_cipher](http://man.openbsd.org/tls_conn_cipher.3)
- [tls_peer_cert_contains_name](http://man.openbsd.org/tls_peer_cert_contains_name.3)
- [tls_peer_cert_subject](http://man.openbsd.org/tls_peer_cert_subject.3)
- [tls_peer_cert_issuer](http://man.openbsd.org/tls_peer_cert_issuer.3)
- [tls_peer_cert_notbefore](http://man.openbsd.org/tls_peer_cert_notbefore.3)
- [tls_peer_cert_notafter](http://man.openbsd.org/tls_peer_cert_notafter.3)
- [tls_peer_cert_hash](http://man.openbsd.org/tls_peer_cert_hash.3)

And many others which you will find in the man pages - The key thing to remember is that
after the handshake, you can use any one of these to compare to expected values to make a
decision about a connection, or log information about it.

You may now proceed to do [Exercise 1b](ex1)

# Certificate Revocation.

Everyone knows certificate revocation is painful and problematic at scale.  Nevertheless there is often value for it in private deployments. You have two choices:

### CRL Support

For basic certificate revocation, you're basically looking at a CRL file, as published by a CA, in PEM format. libtls supports these by setting one up at config time

- [tls_config_set_crl_file](http://man.openbsd.org/tls_config_set_crl_file.3) can be called to load the crl file during configuration. It will be used to check for certificate revocation during the handshake validation.

### OCSP Stapling

Alternatively, OCSP stapling can and will be validated if present in a provided certificate.

- [tls_config_set_ocsp_staple_file](http://man.openbsd.org/tls_config_set_ocsp_staple_file.3) can be called to staple a pre-fetched, DER encoded OCSP server response to a certificate for a handshake.

You can *require* presented certificates to include valid OCSP staples by using

[tls_config_ocsp_require_stapling](http://man.openbsd.org/tls_config_ocsp_require_stapling.3).

at which doing any certificate validations that do not include a valid OCSP staple will fail. If you instead want to verify OCSP stapling manually you can use:

- [tls_peer_oscp_response_status](http://man.openbsd.org/tls_peer_ocsp_response_status.3) on a ctx for which the handshake has completed in order to see what the status is reported by the staple, at which point you can decide if you want to continue or not.

You should now know enough to do [Exercise 1r](ex1)

# Asynchronous IO and event driven programming

libtls is designed to be able to handle asynchronous io through nonblocking descriptors and an event notification mechanism such as poll (or the various kernel event handlers). We'll look at poll(2) here since it's largely portable.

Ad you may have guessed, TLS_WANT_POLLIN and TLS_WANT_POLLOUT relate directly to poll, and the poll events flags *POLLIN* and POLLOUT*

Let's revisit tls_read and tls_write:

- Pay particular attention to the [man page](https://man.openbsd.org/tls_read.3) Example for asynchronous nonblocking io with [poll](https://man.openbsd.org/poll.2). 
  - *TLS_WANT_POLLIN* specifies that the command failed, but needs to be retried using the same arguments, once the the underlying descriptor *READABLE*
  - *TLS_WANT_POLLOUT* specifies that the command failed, but needs to be retried using the same arguments, once the the underlying descriptor *WRITEABLE*
  - Any other negative value indicates a failure.

Unlike the synchronous IO case, where you can simply retry the same operation immediately, In the asynchronous, nonblocking case you need to wait and use poll to fine out when the descriptor is readable, or writeable again.

This can seem very counter-intuitive when you are doing a write operation, but need to set the descriptor to POLLIN to fine out if it is readable before doing the write again.

This occurs because tls_read, tls_write, and tls_handshake are *NOT* system calls, and will not exclusively read or write depending on what is happening at the TLS layer underneath. As an example, you could be doing a handshake automatically during a read or write, or do renegotiation.

Finally you should have enough to proceed onto [Exercise 2](ex2) Exercise 2 is a new program
for you to convert - an "echo" client and server using poll() on both ends. Your goal here is
to get as far with this program as you can in the synchronous case in exercise 1a - so he client
can anonymously connect to the server, validate the cert and do full tls.

# Further Reading and Other Resources

A number of programs in OpenBSD use libtls, examining the source code is often a useful resource

- [nc](http://man.openbsd.org/nc.1 - [Source code for OpenBSD nc](https://github.com/openbsd/src/tree/master/usr.bin/nc)
- [httpd](http://man.openbsd.org/httpd.1 - [Source code for OpenBSD httpd](https://github.com/openbsd/src/tree/master/usr.sbin/httpd)
- [acme-client](http://man.openbsd.org/acme-client.1 - [Source code for OpenBSD acme-client](https://github.com/openbsd/src/tree/master/usr.sbin/acme-client)
- [acme-client](http://man.openbsd.org/ocspcheck.8 - [Source code for OpenBSD ocspcheck](https://github.com/openbsd/src/tree/master/usr.sbin/ocspcheck)
- [OpenBSD's libtls regression tests] https://github.com/openbsd/src/tree/master/regress/lib/libtls)

































LocalWords:  oscp io nonblocking nc httpd ocspcheck
