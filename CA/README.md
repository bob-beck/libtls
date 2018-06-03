
### Happy Bob's Test CA

This implements a dumb CA with a root, intermediate, and ocsp signer using the
openssl command..

While I am ok with you learning from it, please remember:

## Friends do not let friends use the openssl(1) command in production!

It's nasty and horrible. it doesn't check return values. for the love of Cthulhu don't trust it for anything real - I am using it here for *TESTING*, and that's all you should do with it.  You've been warned.

