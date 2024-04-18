"C:\Program Files\OpenSSL-Win64\bin\openssl.exe" req -new -newkey rsa:4096 -days 365 -nodes -x509 ^
    -subj "/C=GB/ST=London/L=London/O=Global Security/OU=R&D Department/CN=192.168.20.69.com " ^
    -keyout cert.key -out cert.crt