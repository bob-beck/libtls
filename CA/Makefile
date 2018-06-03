all: chain.pem intermediate/certs/ocsp-localhost.pem

clean:
	/bin/rm -rf root intermediate chain.pem

intermediate/certs/ocsp-localhost.pem: intermediate/certs/intermediate.cert.pem
	(cd intermediate && openssl genrsa -out private/ocsp-localhost.key.pem 4096)
	(cd intermediate && openssl req -batch -config openssl.cnf -new -key private/ocsp-localhost.key.pem -subj "/C=CA/ST=Edmonton/O=Bob Beck/OU=LibreSSL Tutorial OCSP division/CN=localhost" -out csr/ocsp-localhost.csr.pem)
	openssl ca -batch -config intermediate/openssl.cnf -extensions ocsp -days 375 -notext -md sha256 -in intermediate/csr/ocsp-localhost.csr.pem -out intermediate/certs/ocsp-localhost.pem

chain.pem: intermediate/certs/intermediate.cert.pem root/certs/ca.cert.pem
	cat intermediate/certs/intermediate.cert.pem root/certs/ca.cert.pem > chain.pem

root/certs/ca.cert.pem:
	mkdir -p root/private
	mkdir -p root/certs
	mkdir -p root/newcerts
	cp openssl-root.cnf root/openssl.cnf
	(cd root && openssl genrsa -out private/ca.key.pem 4096)
	touch root/index.txt
	echo 1000 > root/serial
	(cd root && openssl req -batch -config openssl.cnf -key private/ca.key.pem -new -x509 -days 7300 -sha256 -extensions v3_ca -subj "/C=CA/ST=Edmonton/O=Bob Beck/OU=LibreSSL Tutorial/CN=Root CA Cert" -out certs/ca.cert.pem)

intermediate/certs/intermediate.cert.pem: root/certs/ca.cert.pem
	mkdir -p intermediate/certs
	mkdir -p intermediate/crl
	mkdir -p intermediate/csr
	mkdir -p intermediate/newcerts
	mkdir -p intermediate/private
	cp openssl-intermediate.cnf intermediate/openssl.cnf
	touch intermediate/index.txt
	echo 1000 > intermediate/serial
	echo 1000 > intermediate/crlnumber
	(cd intermediate && openssl genrsa -out private/intermediate.key.pem 4096)
	(cd intermediate && openssl req -batch -config openssl.cnf -key private/intermediate.key.pem -new -sha256 -subj "/C=CA/ST=Edmonton/O=Bob Beck/OU=LibreSSL Tutorial/CN=Intermediate CA Cert" -out csr/intermediate.csr.pem)
	openssl ca -batch -config root/openssl.cnf -extensions v3_intermediate_ca -days 3600 -notext -md sha256 -in intermediate/csr/intermediate.csr.pem -out intermediate/certs/intermediate.cert.pem


