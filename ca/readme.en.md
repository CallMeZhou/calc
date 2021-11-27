# SSL Basics

It doesn't require much brain power.

It is easy.

## The overall method

### What are SSL and symetric encryption?

SSL is a technology that prevents the third person from hearing the conversation between two peers.

Theoretically, a symetric encryption should work. A symetric encryption uses one key to encrypt and decrypt information.

Suppose we use XOR for encryption, the key is 0101, and the clear text is 0011. We can use the same key to all the work.

```
0011            xor            0101     = 0110
^^^^plaintest   ^^^algorithm   ^^^^key    ^^^^cyphertext

0110            xor            0101     = 0011
^^^^cyphertext  ^^^algorithm   ^^^^key    ^^^^plaintext
```

This is a naive but typical symetric encryption.

**Please bear in mind: SSL uses symetric encryption to encrypt the content of the communication! When a web browser talks to an HTTPS server and when the server responses, they hold the same key for encoding and decoding.**

They use much more advanced maths than XOR. However, even if they used XOR, would you decrypt it without knowing the key?

### That's it? No, of course.

Countless web browser instances are talking to countless HTTPS servers every second in the world. **It's just impossible for every pair of peers to have the same encryption key in advance!**

You cannot preinstall the key of Facebook in your laptop, so what do we do?

**How about the server sends the key to the browser right after the connection is established? YES, that is what we have to do!**

But the key cannot be encrypted and the third person is still listening. Shhh!

So, how does the server safely sends the key?

### Asymetric encrypton

An asymetric encrypton uses TWO keys 