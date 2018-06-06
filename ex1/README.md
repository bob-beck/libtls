
# TLS this program!

As you may note, what is in here is exactly what you got for the review exercies

The reason is simple, your task here is to make the client and server here speak
TLS!

### Exercise 1:

For a first step Make the client connect anonymously, and validate the server's certificate.

- Use the root certificate from ../CA/root.pem as the root of trust
- Use the server certificate from ../CA/server.[key|crt] as the server certificate

Stop at the point that you get the client talking to the server. Other exercises below
will build upon this.

### Exercise 1r

###CRL use
For certificate verification, modify your program so that the client supports a CRL list.

The CRL for your certificates is located in ../CA/intermediate/crl/intermediate.crl.pem.

Start your server up using ../CA/revoked.[key|crt] instead of server.crt. Try to connect to it when using the CRL and not using the CRL.

###OCSP Stapling

Modify your server to use an OCSP staple file. You can fetch an ocsp staple for your server.crt
by running the ocsp server in ../CA/ocspserver.sh, then connecting to it using ../CA/ocspfetch.sh server.crt - which will retreive a server.crt-ocsp.der file  This can be used as the input file for [tls_config_set_ocsp_staple_file](http://man.openbsd.org/tls_config_set_ocsp_staple_file.3)

Modify your client to check for the OCSP staple, or to require it. - connect up and see what happens.

If you have time you can try the same thing with the revoked certificate in the server.







