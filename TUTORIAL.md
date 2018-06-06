
# Happy Bob's Libtls tutorial

libtls is shipped as part of libressl with OpenBSD. It is designed to be simpler to use than
other C based tls interfaces (especilly native OpenSSL) to do "normal" things with TLS in programs.

### So what's libtls good for?

- Any C program that needs to use TLS with client or server side certificate verification using common methods

What is libtls not good for?

- Arbitrary crypto operations.
- Certificate mangling or generation
- Being your own CA
- Implementing Kerberos
- Using a different interface to sockets to talk to sockets.
- etc etc.

In a nutshell, libtls is designed to do the common things you do for making TLS connections easy. It is not designed to be a swiss army knife of everything possible.

### Goals for this tutorial

So if you go through this, what I hope you get out of this is:

- A basic review of sockets in C, with read and write and synchonous IO. Do this in ex0

- How to convert a basic client and server program to use tls instead of cleartext in ex1

- How to convert a more advanced client and server using asynchronous io and poll() in ex2

- Enough libtls knowledge to know where to find more, and extend this into your own work.

### Basic libtls

libtls programs use a few opaque structures:

- *struct tls_config* : a tls configiration, used in turn to make tls connections.
- *struct tls* : a tls ctx, all the context for a tls connection.

### libtls setup. 

Typical initialization for a program will look like a minimun of:

- Optionally call [tls_init](https://man.openbsd.org/tls_init.3) to initialize the library
- Call [tls_config_new](https://man.openbsd.org/tls_config_new.3) to set up a new tls configuraiton
- Call [tls_config_set_ca_file](https://man.openbsd.org/tls_config_set_ca_file.3) to add your roots o
- Optionally Call [tls_config_set_cert_file](https://man.openbsd.org/tls_config_set_cert_file.3) to add your own certificate
- Optionally Call [tls_config_set_cert_key_file](https://man.openbsd.org/tls_config_set_cert_key_file.3) to add your certificate key

Once this is done you have a configuration set up to potentially initiate or receive tls connections. to make use of that configuation you need to

- Get yourself a tls context using either
  - [tls_server](https://man.openbsd.org/tls_server.3) to set up a server context
  - [tls_client](https://man.openbsd.org/tls_client.3) to set up a client context

Once you have this you apply a configuration to a context using

- [tls_configure](https://man.openbsd.org/tls_configure.3) to take your server or client context, and apply the configuration to it.

Now you're actually ready to use TLS.









