# SSL Basics

The "basics" doesn't require much brain power.

The "basics" is easy.

## The theory

### What are SSL and symetric encryption?

SSL is a technology that prevents the third person from hearing the conversation between two peers. Theoretically, a symetric encryption should work well for the this goal.

It uses the same key to encrypt and decrypt information. The XOR operation is a naive but typical example.

Suppose we have a key 0101, and the plaintext is 0011. Using XOR, we can encrypt and decrypt the data by the same key:

```
0011            xor            0101     = 0110
^^^^plaintest   ^^^algorithm   ^^^^key    ^^^^cyphertext

0110            xor            0101     = 0011
^^^^cyphertext  ^^^algorithm   ^^^^key    ^^^^plaintext
```

This is symetric encryption, a.k.a, public-key encryption.

**Please bear in mind: SSL uses symetric encryption to protect the communication! When a web browser talks to an HTTPS server and when the server responses, they hold the same key for encoding and decoding.**

They use much more advanced mathematics than XOR. However, even if they used XOR, would you decrypt it without knowing the key? So you should believe this: providing the key is not leaked, the symetric encryption is reliable enough.

### That's it? No, of course.

Countless web users are talking to countless HTTPS servers every second in the world. **It is just impossible for every pair of peers to have the same encryption key in advance!**

You cannot pre-install the key of Facebook in your laptop, so what do we do? Simple --- **the browser sends a random key to the browser right after the connect is established.** After that, each peer encodes the data package by the key before sending out, so the data going through the network is cyphertext, which means, in either direction, the communication is reliably protected by the symetric encryption --- well, if the key is not leaked.

### That's it? No, of course.

The key cannot be encrypted but the third person is still listening. He can intercept anything, including the key, so, how does the client safely sends the key to the server?

### Asymetric encrypton

Let's put the previous question aside for a moment and take a quick look at the asymetric encrypton first.

An asymetric encryption has 2 keys in a pair. The game rule is that, if the information is encrypted by one of the keys, it can and only can be decrypted by the other key of the same pair. It does not matter which key is used for encryption.

A super easy example is using "add". I know you must be laughing as if I am drunk. But think of this example: suppose we have a plaintext 8, and we have a pair of keys +2 and -2, so we can do:

```
8  + (+2) = 10 // encryption
10 + (-2) = 8  // decryption

or:

8  + (-2) = 6  // encryption
6  + (+2) = 8  // decryption
```

The above example look silly, but it gives you some intuitive "feeling" about an asymetric encrypton. In the real world, we do use much more complex mathematics, but the workflow is the same.

How do we apply asymetric encrypton in the communication between the users and the HTTPS servers?

Now imagine: after the connection is established, the server starts to talk first. It holds a pre-created key pair and sends out one of them to the web broswer while holding the other secrectly in hand. Now that, each peer holds one key of the key pair. Each peer encrypts the data using the key before sends out. When the other peer receives the data, it can surely decrypts the data because it is holding the other key.

But our lovely third person comes again. When the server sends out one of the keys to the browser, the third person intercepts it happily. Now please think about this question: if it happens, is the communication still safe?

Yes and no, right?

The third person has the same key as the browser does, so it can receive and decrypt anything the browser can do, however, it CANNOT decrypt the data sent from the browser to the server, because the key held by the server is never leaked.

I know you will ask --- half safety is still unsafe, so it is still useless.

No, this is not a correct attitude for life. You should always look at the bright side --- the data from the browser to the server is still protected even if one of the key is leaked.

We asked the question in the above paragraph: how does the client safely sends the key to the server? Actually, you have already got the answer!

### Let's go over the whole workflow again!

Let's put all the pieces mentioned above together:

First, we create a key pair for the server. After the TCP connection is established, the server sends one of the keys to the client and holds the other key in hand. Conventionally, the key sent out to the client is called the `public key` and the other one hold by the server is called the `private key`. The client generates a random key, which is usually called the `symetric key`. The client uses an asymetric algorithm to encrypt the `symetric key` by the received `public key`. Now you should realize a fact: only the server can decrypt the encoded `symetric key`. The client, then, sends the `symetric key`'s cyphertext to the server. Finally, the server uses the `private key` to decrypt the `symetric key`. From now on, the asymetric algorithm, the `public key` and the `private key` are no longer needed, and the 2 peers will continue communicating using the symetric algorithm and the `symetric key`. The third person can only intercept the `public key` and the `symetric key`'s cyphertext, but he cannot decrypt it, so he cannot decrypt the following conversion.

### That's it? Emmm... almost. Just one more thing.

In the above paragraph, the third person was locked out of the communication because he cannot decrypt the `symetric key`'s cyphertext. That is becuse he has the same `public key` that the browser has, but he has no `private key`.

But a smarter third person might hack in by the following way:

When the server sends out the `public key`, he intercepts it and keeps it, never sends it again. He also have a key pair! Then he sends his `public key` to the browser and waits. The browser, as mention previously, uses the received false `public key` to encrypt the `symetric key` and sends the cyphertext out. When the third person intercepts it again, now the `symetric key` is no longer a secret to him. Then he uses the true `public key` to encrypt the `symetric key` again and passes it out to the server. Now that you know what's gonna happen.

Why can the third person still hack in? Because the browser cannot know if the `public key` has been changed or not, so it leaves the third person a chance to exchange his false `public key` with the real one. Is it possible to let the browser confirm what it receives is really from the server?

### The last piece --- digital signature, certificate, and Certificate Authority (CA).

Remeber what problem is be resolved --- the browser needs a way to confirm what it receives is from the server, changed by no one.

To resolve this, we need one or more Certificate Authorities (CA). A Certificate Authority is an organization (a company, or an NPO). We cannot pre-install website's key, but we can (and have to) pre-install the Certificate Authorities' keys!

Let's look at the web server's `public key` again. The browser need to authenticate it. The website's developer will send its `public key` to a CA. CA uses a hash function to find the `hash code` of the `public key`. Then, CA uses its `private key` to encrypt the `hash code`. The cyphertext of the `hash code` is called `digital signature`. The combination of the web server's `public key` in plaintext and the `hash code` in cyphertext is called `certificate`.

At the beginning of the communication, the web server sends the `certificate` out to the browser. The browser uses the same hash function to find a `hash code`. Now that, if the browser can decrypt the `digital signature`, then it will have `hash code` created by the CA. If these 2 `hash codes` are identical, it means no one changed anything in the `certificate`.

As mentioned, the `digital signature` was encrypted by the CA's `private key`, so does what browser need now? --- the CA's `public key`.

Most web browsers are released with a lot of pre-installed `public keys` of well-known CAs. In case a web server has a `certificate` containing a `digital signature` created by an unknown CA, you must install the CA's `public key`. This is usually unsafe for normal users. Microsoft Windows will prompt you warning message 2 times before a CA's `public key` is installed.

But if you are developing your own web server, you usually will not bother asking a CA company for a `digital signature` as it is usually not for free. In this case, you need to create your own CA key pairs and use it to sign your web server's key pairs, and install your own CA's `public key`. We will see how to do this in the following section.

## Create key pairs for test CA and test web server

No worries. It is much much much easier than the above paragraphs. Just keep calm and carry on.

In general, what we will do is to:

1. Create 2 key pairs. One pair for the test CA, the other pair for the test web server. (i.e., for our CALC server)
2. User the CA's key pair to sign the web server. To be more precise: use the CA's `private key` to turn the web server's `public key` into a `certificate`.
3. Install the CA's `public key` into your browser (if you use Mozilla Firefox) or install it into your OS (if you use other broswers).

