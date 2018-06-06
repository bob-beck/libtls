
# Happy Bob's Test CA

This implements a dumb CA with a root, intermediate, and ocsp signer using the
openssl command..

While I am ok with you learning from it, please remember:

### Friends do not let friends use the openssl(1) command in production!

It's nasty and horrible. it doesn't check return values. for the love of Cthulhu don't trust it for anything real - I am using it here for *TESTING*, and that's all you should do with it.  You've been warned.

Now having said that. the Makefile in here sets everyting up.

    - "make" builds the root CA, intermediate, and ocsp signer, along with a server and client certificate, bothe having a CN for "localhost"
    - "make clean" blows away *everything* including the signers and issued certs. Don't do this if you want to keep using the same certs.
    -  "makecert.sh" is a little shell script that can be use to make client and server certs with an arbitrary CN and email address
    -  "ocspfetch.sh" retreives the OCSP response for server.crt using OpenBSD's oscpcheck
    -  "ocspfetch.sh.openssl" retreives the OCSP response for server.crt using openssl commands
    
