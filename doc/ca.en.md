# SSL quick guide

It is easy, in the first place.

In this doc, I hope to clarify 2 things for you:

1. The basics of SSL --- not the mathematics but the workflow.
2. How do you configure the server and the browser to make your test site support HTTPS.

## Symmetric encryption

SSL is a technology that prevents the third person from hearing the conversation between two peers. Both symmetric and asymmetric encryption are used at different stages by SSL.

Symmetric encryption only uses ONE KEY. When a piece of plaintext is encoded by a key, you get a piece of cyphertext, which can only be decoded by the same key. **Symmetric encryption can be very reliable, but you have to find a safe way to send the key to the one who should read your message.**

The XOR operation is a typical example for symmetric encryption. Suppose we have a key 0101, and the plaintext is 0011. Using XOR, we can encrypt and decrypt the data by the same key:

```
0011            xor            0101     = 0110
^^^^plaintest   ^^^algorithm   ^^^^key    ^^^^cyphertext

0110            xor            0101     = 0011
^^^^cyphertext  ^^^algorithm   ^^^^key    ^^^^plaintext
```

## Asymmetric encryption

Asymmetric encryption uses a KEY PAIR. When the plaintext is encoded by one of the keys, it can only be decoded by the other key in the same pair. It doesn't matter which one is used as the encoder or decoder. The advantage is that, if one of the key is leaked, 

A super easy example of asymmetric encryption is using "add". I know you must be laughing. But think of this example: suppose we have a plaintext 8, and we have a pair of keys +2 and -2, so we can do:

```
8  + (+2) = 10 // encryption
10 + (-2) = 8  // decryption

or:

8  + (-2) = 6  // encryption
6  + (+2) = 8  // decryption
```

Hey, I know the above exsamples look really silly. I just wanted to give you an intuitive example of those 2 encryption methods.

**Using asymmetric encryption, you must send one of the keys to your peer and hold the other one in hand. The one you give out is called the `public key` and the other one is called the `private key`.**

Since the asymmetric encryption requires to send out the key too, what makes it different from the symmetric encryption?

> Symmetric encryption only uses ONE KEY. Once it is leaked, there is no secret.
> Asymmetric encryption uses TWO KEYs. If only one is leaked, there is still one direction of the communication is safe.

Please take a look at the silly "add" example again. If I hold the key `-2` and give the key `+2` to you, and if the `+2` is known by others, then when I send the cyphertext `6` to you, both you and the others can decode it and get the plaintext `8`, right?

But since the others do not know the other key `-2`, if you send the cyphertext `10` to me, only I can decode it because only I know the key `-2`.

That why I wrote "*If only one is leaked, there is still one direction of the communication is safe*".

## Are you thinking what I am thinking?

If you are lucky, you should have already connected the following 2 points in your mind:

1. The symmetric encryption needs a safe way to send the key to the message receiver.
2. The asymmetric encryption can ensure at least one direction of the communication to be safe.

Now you should realized how SSL might work. Let me make it very clear to you. Suppose we have an HTTPS server and a web browser. They establish the connection in the following sequence:

1. The browser connects to the server;
2. The server accepts it and immediately sends the `public key` to the browser in plaintext;
3. The browser generates a random string as the `symmetric key`, encodes it by the server's `public key` using asymmetric algorithm;
4. The browser sends the `symmetric key` to the server in cyphertext;
5. The server decodes the `symmetric key` using its `private key`;
6. From now on, both ends encodes and decodes the messages by the `symmetric key` using symmetric algorithm.

Suppose the third person can intercept what the browser can receive, so he will receive the `public key`. If the server would encode something using its `private key` and send to the browser, the third person would be able to intercept and decode, but unfortunately, it will never happen, so the poor guy can hack nothing out.

This is how SSL protects the communication in both directions.

## That's it? Emmm, wait...

If you were the third person in the above story, would you hang white flag? Wait, you still have a chance! "*There will be no white flag above your door*", right?

Now that suppose your name is Mr. Third, please consider the following sequence:

1. The server sends the `public key` to the browser in plaintext;
2. You intercept the whole message package;
3. You keep the server's `public key` and sends your `public key` to the browser;
4. The browser encodes the `symmetric key` by your `public key` and sends it back;
5. You intercept the whole message package again;
6. You decode the `symmetric key` using your `private key` because it was encoded using your `public key`;
7. You make a copy of the `symmetric key` and encode it using the server's `public key` and pass it on;
8. Finally, the silly server and the browser start to communicate back and forth using the `symmetric key`, and they are just streaking in front of you.

## The last day of the third person

In the above stroy, the problem is that the brower does not know if the `public key` is the one sent from the server. In other word, it cannot authenticate the key it receives.

Is the authentication possible for the browser? Yes, it's quite straight forward than you might expect. 

Please close your eyes and go over the asymmetric encryption again...

Suppose you established a company:

- You have a key pair for asymmetric encryption;
- All websites in the world give their `public keys` to you;
- You encrypt their keys using your `private key` and return them their `public keys` in cyphertext;
- Now each website has TWO `public keys`. They are actually the same key, but one in plaintext, the other in cyphertext;
- When the browser connects, the web server sends out the TWO copies of the same `public keys` together;
- The browser receives them --- if it can decode the `public keys` in cyphertext, it will have TWO identical keys, right?

Tell me, what the brower needs in order to decode it? --- Your company's `public keys`!

Now, suppose you pre-install your `public keys` into the broswer, then it now can validate if the web server's `public keys` is the original one: it decodes the web server's `public keys` in cyphertext using your `public keys`, and then it compares if the decoded key is identical to the plaintext key.

You should have enough knowledge to think about in your head --- the third person cheated the browser by replacing the web server's `public keys` with his own `public keys` and the browser couldn't validate the authenticity of what it receives. Now, can he still do that? Think of it by your own.

## The last piece of SSL's world

In the real world, "your company" mentioned in the above story is called **"Certificate Authority" or "CA"**. There are some in the world but not that many. Just over 100 or so.

Instead of encrypting the whole `public key`, the CA companies find a short hash code of the web site's `public key` and just encode the hash key. **The encoded hash key is called the "digital signature"**. **The web site's `public key` and the `digital signature` are put together in one file**, which is **called the "certificate"**.

The CA companies' `public keys` are pre-installed in browsers and some operating systems. No worries, the third person still doesn't have the `private keys` so they still cannot fake the certificate! It's safe.

## Create key pairs for test CA and test web server

**The example commands in the following content is for Linux. If you are using Windows, please use Google.**

Now we have come to the easiest part of this doc.

If you have your own test web server running on your computer and you want it to talk to the browsers in HTTPS, you need to do the following:

1. Create 2 key pairs. One pair for the test CA, the other pair for the test web server. Let's say: pair-CA {private-CA, public-CA}; pair-site {private-site, public-site}.
2. Use {private-CA} to turn the {public-site} into a `certificate`.
3. Install the {public-CA} into your browser (if you use Mozilla Firefox) or install it into your OS (if you use other broswers).
4. Tell your web server where to find the {private-site} and the `certificate`.

Done.

### Let's practice it

1. Prepare 2 folders:

```sh
# ca/ will save the private key and public key for the test CA company
mkdir ca
# ca/issued/ will save the private key, public key, and the certificate for the test web site
mkdir ca/issued
```

2. Create 2 key pairs:

Create the key pair for the test CA company:

The command:

```sh
openssl req -nodes -new -x509 -keyout ca.key -out ca.pem
```

Demo:

```sh
agedboy@5R3RZY2:~/dev$ cd ca
agedboy@5R3RZY2:~/dev/ca$ openssl req -nodes -new -x509 -keyout ca.key -out ca.pem
Generating a RSA private key
..........................+++++
.......+++++
writing new private key to 'ca.key'
-----
You are about to be asked to enter information that will be incorporated
into your certificate request.
What you are about to enter is what is called a Distinguished Name or a DN.
There are quite a few fields but you can leave some blank
For some fields there will be a default value,
If you enter '.', the field will be left blank.
-----
Country Name (2 letter code) [AU]:
State or Province Name (full name) [Some-State]:
Locality Name (eg, city) []:
Organization Name (eg, company) [Internet Widgits Pty Ltd]:
Organizational Unit Name (eg, section) []:
Common Name (e.g. server FQDN or YOUR name) []:<this is sort of important>
Email Address []:
```

When creating the private key and public key for the test CA company, openssl will ask you for some information. You can leave all fields blank by input just a dot, but the "Common Name" is important, because it will be seen in the certificate list in your browser or OS. When you want to remove the certificate after your experiment is done, you will need this name to identify the test certificate.

Create the key pair for the test web site:

The command:

```sh
openssl req -nodes -new -keyout site.key -out site.req
```

Demo:

```sh
agedboy@5R3RZY2:~/dev/ca$ cd issued
agedboy@5R3RZY2:~/dev/ca/issued$ openssl req -nodes -new -keyout site.key -out site.req
Generating a RSA private key
...........+++++
....................+++++
writing new private key to 'site.key'
-----
You are about to be asked to enter information that will be incorporated
into your certificate request.
What you are about to enter is what is called a Distinguished Name or a DN.
There are quite a few fields but you can leave some blank
For some fields there will be a default value,
If you enter '.', the field will be left blank.
-----
Country Name (2 letter code) [AU]:
State or Province Name (full name) [Some-State]:
Locality Name (eg, city) []:
Organization Name (eg, company) [Internet Widgits Pty Ltd]:
Organizational Unit Name (eg, section) []:
Common Name (e.g. server FQDN or YOUR name) []:localhost
Email Address []:

Please enter the following 'extra' attributes
to be sent with your certificate request
A challenge password []:811020
An optional company name []:811020
```

This time, the public key file is created for being signed.

3. Sign the site's public key:

First, create a text file "site.ext" with the following content in the "ca/issued" folder:

```
authorityKeyIdentifier = keyid,issuer
basicConstraints = CA:FALSE
keyUsage = digitalSignature, nonRepudiation, keyEncipherment, dataEncipherment
subjectAltName = @alt_names

[alt_names]
DNS.1 = localhost
IP.1 = 127.0.0.1
DNS.2 = www.calc-server.com
IP.2 = 192.168.0.112
```

You can find this file in the "/ca/localhost" folder of this project.

Second, sign the site's public key.

The command:

```sh
openssl x509 -req -CAcreateserial -days 3650 -sha256 -in site.req -CA ../ca.pem -CAkey ../ca.key -extfile site.ext -out site.crt
```

Demo:

```sh
agedboy@5R3RZY2:~/dev/ca/issued$ openssl x509 -req -CAcreateserial -days 3650 -sha256 -in site.req -CA ../ca.pem -CAkey ../ca.key -extfile site.ext -out site.crt
Signature ok
subject=CN = localhost
Getting CA Private Key
```

That's it.

"site.key" is the private key for the test web site, "site.cert" is the certificate file of the site.

"ca.pem" is the test CA company's public key that you need to install to your browser or OS.