
### Exercise 2 - nonblocking IO with poll, and TLS

Again, familiarize yourself with the client and echo server in this directory. 

Just as before for 2, make the client connect anonymously, and validate the server's certificate.

- Use the root certificate from ../CA/root.pem as the root of trust
- Use the server certificate from ../CA/server.[key|crt] as the server certificate

If you get that far, you can continue to play and bring in other validation steps as per the previous pieces of exercise 1.
