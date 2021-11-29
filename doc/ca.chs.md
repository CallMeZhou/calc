# SSL快速向导

首先，它很简单。

本文只介绍两件事：

1. SSL基础知识（不包括其中的数学知识，只涉及工作流程）；
2. 如何设置测试服务器和浏览器，使它们能够通过HTTPS通信。

## 对称加密

**SSL的目的就一个：防止两个人之间的对话被第三者偷听。**

为了达到这个目的，SSL在通信的不同阶段，使用了对称加密，和非对称加密两种方法。

**对称加密只使用一把钥匙：它既被用来加密，也被用来解密。**当一段信息的“明文”被一把钥匙加密之后，所获得的“密文”只能用同一把钥匙解开。

**对称加密很可靠，但有个前提：既然加密、解密双方必须使用同一把钥匙，那么怎么安全地把钥匙，从一方手里交到另一方手里呢？**

什么？你说通信双方事先商量好，然后各自把钥匙写到程序里就行？

不行，世界上有无穷多的网站你可以访问，既不能让所有网站都用同一把钥匙（那就等于没有钥匙了），也不可能让你的浏览器事先把世界上所有网站的钥匙都事先写在程序里。所以，只能当你跟网站连上之后，由其中一方每次产生一把随机钥匙，并发送到另一方手里。**但只要一发送，就会泄露给第三者。**

想象一下异或运算（xor），它就是个典型的对称加密算法的例子。（只是个例子啊，现实中不会真去用它。但你能想到如何破解异或加密吗？想想看……）

## 非对称加密

**非对称加密使用一对钥匙，也就是相互关联的两把钥匙。当明文用其中一把钥匙加密后，获得的密文能且只能用同一对中的另一把钥匙解密。**非对称加密的好处是：如果其中一把钥匙泄露了。则两个方向的通信中，至少还有一个方向是安全的。

比如，有一对钥匙，我手里留一把（通常称为“私钥”），送给你一把（通常称为“公钥”）。结果给你的过程中，被第三者窃取了。那么我用我手里的钥匙加密后的信息，你和第三者都可以解开。但是你用你收到的钥匙加密的信息，只有我能解开，第三者解不开。

有一个特弱智的例子：假如+2和-2是一对钥匙，那么用加法，就可以对一个整数进行非对称加密，对吧？只是为了给你个直观的感觉。

好了，我估计你现在会想一个问题：

> 既然非对称加密也需要将一把钥匙发送给对方，那么它跟对称加密一样，有泄露钥匙的风险。
> 虽然钥匙泄露后，还有一个方向的通信是安全的，但双向通信废了一半，又有什么用呢？

## 你想到了吗？

幸运的话，你可能已经将下面两件事联系起来了：

1. 前文说，对称加密很可靠，但需要找到一个方法，安全地将钥匙从通信一方发送给另一方；（不论从哪一方送给哪一方都行）
2. 非对称加密中，如果其中一把钥匙泄露了。则双向通信中，至少还有一个方向是安全的。

这不就好了，至少还安全的那个通信方向，就可以用来发送那把“对称钥匙”啊！

**现在我们来复盘一下整件事。**

假设用浏览器访问网站，它们如何建立加密连接？

1. 浏览请求连接服务器；
2. 服务器接受请求，**并当即发送一把非对称钥匙给浏览器**；（再次：通常称为“公钥”）
3. 浏览器随机生成一个字符串，作为“对称钥匙”，并对它用收到的服务器“公钥”进行非对称加密；
4. 浏览器将加密的“对称钥匙”发还给服务器；
5. 服务器用手里的“私钥”进行解密，获得“对称钥匙”；
6. 从现在开始，服务器和浏览器都使用“对称钥匙”，通过对称加密算法来进行加密通信。而那一双非对称钥匙不再使用。

上面6步能防住第三者吗？咱们想一下：

假设第三者黑进了通信链路，那么他首先会截获到服务器“公钥”。这时，如果服务器用它的“私钥”加密信息并发给浏览器，则第三者可以解密，但可惜这件事根本不会发生。接下来浏览器将“对称钥匙”用“公钥”并还给服务器，第三者截获后，由于没有“私钥”，因此不能解密获得“对称钥匙”。接下来的通信完全使用“对称钥匙”加密，这样第三者就完全无法解密并窃取信息。

这就是SSL实现加密通信的方法！

## 就这？等一下……

如果你是上面故事中的第三者，你要举白旗吗？且慢，还有一线希望。请考虑下面步骤：

1. 建立连接，服务器向浏览器发送“公钥”；
2. 你截获公钥；
3. 你把服务器的“公钥”扣下，并把你自己的“公钥”发送给浏览器;
4. 浏览器用收到的“公钥”会它的“对称钥匙”进行加密并发送；
5. 你截获加密的“对称钥匙”；
6. 由于那是用你的“公钥”加密的“对称钥匙”，因此你可以用自己的私钥解密，或者真正的“对称钥匙”。
7. 由于你扣下了服务器的“公钥”，因此你可以再对“对称钥匙”用服务器的“公钥”加密，这样再发送给服务器后，它可以顺利解密；
8. 这一通操作下来，服务器、浏览器和你便都有了“对称钥匙”，成为了无话不说的一家人。

## 第三者的末日

上面故事中的漏洞在于：浏览器收到一把“公钥”之后，它无法知道有没有被“调包”，无法验证这是不是服务器发的那把。

我们需要一个方法，让浏览器可以对收到的“公钥”验明正身。方法是有的，比你想象的要简单直接。

现在请你闭上眼睛，回忆一下“非对称加密”的游戏规则。

好了，假设你开了一家公司，并开展如下业务：

- 你的公司有一对“公钥”和“私钥”，可以用于非对称加密；
- 世界上所有网站都把自己的“公钥”送给你；
- 你把他们送来的“公钥”用你的“私钥”加密，并把密文还给网站；
- 现在，每家网站都有两把本质上相同的钥匙（公钥），一把是明文，一把是密文；
- 当网站服务器和浏览器建立连接后，它把“公钥”的明文和密文一起发送出去；
- 这时，浏览器会收到服务器“公钥”的一明一密两份拷贝。试想：如果浏览器拥有你的公司的“公钥”，那么它就能够对那份密文进行解密，于是便可获得两份完全相同的明文钥匙。但凡第三者对数据进行修改或调包，那么浏览器用你的公司的“公钥”解密出来的数据，两份就会对不上。（提示：任何人，包括浏览器和第三者，都没有你的公司的“私钥”，因此第三者如果调包了明文钥匙，他无法对明文钥匙生成正确密文！有那么一点点绕，请停下来想一想，万一不行就在纸上画一画）

好了，到这一步为止，第三者彻底歇菜了。

## 进入SSL安全世界的最后一步

在现实世界中，上文提到的“你的公司”，就是所谓的“CA公司”。世界上虽然有不止一家，但目前最多也就百十来家，并不太多。世界上任何一款浏览器或者操作系统，在发行时都预装了所有已知CA公司的“公钥”。

不过，在实际操作中，CA公司并不会直接对网站提交的“公钥”进行解密，而是先对“公钥”计算一个较短的哈希串，然后对这个短哈希串进行加密。记住：这个加密的公钥哈希字符串，叫做“数字签名”。将“数字签名”与“公钥”明文保存在一个文件中，该文件叫网站的“证书”。

请再在心里重复一遍：网站有一个“证书”，证书里有网站的“公钥”的明文一份，还有该“公钥”的哈希值的密文一份。该密文是使用CA公司的“私钥”加密的。世界上任何人都**不知道**CA公司的“私钥”，世界上任何人都**知道**CA公司的“公钥”，因此所有人都可以对“数字签名”进行解密，但是没有人能够模仿CA公司，生成可以被CA公司的“公钥”解密的伪“数字签名”。

从此世界归于平静。

## 最后一个实操议题：在实验环境中进行实验


**下面命令是针对Linux的。非Linux用户可自行上网搜索。相关帖子很多。**

下面我们完成本文最轻松的部分：创建一个实验CA公司，为一个实验网站创建“证书”。

我们需要做如下几件事：

1. 创建两对钥匙（也就是4把）。一对属于实验CA公司，另一对属于实验网站。为了方便，我们称它们为：CA公钥、CA私钥、网站公钥、网站私钥；
2. 使用CA私钥将网站公钥变成证书；
3. 将CA公钥安装在浏览器（对火狐用户）或者安装在操作系统上（对非火狐用户）；
4. 将网站私钥和证书加载到网站服务器中。

齐活。

### 来一把呗？

1. 先创建两个文件夹：

```sh
# ca/ 用于存放CA公钥、CA私钥
mkdir ca
# ca/issued/ 用于存放网站公钥、网站私钥和网站的证书（有了证书后，网站公钥其实就没用了）
mkdir ca/issued
```

2. 创建两对钥匙：

为实验CA公司创建公钥和私钥：

命令:

```sh
openssl req -nodes -new -x509 -keyout ca.key -out ca.pem
```

举例:

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

执行上面命令后，openssl会让你填写一堆信息，国名、所在地名等等，在实验环境里它们都不重要，填个“.”号然后直接回车即可。但是“Common Name (e.g. server FQDN or YOUR name)”这一项，你最好填一个记得住的、容易识别的字符串，比如你名字的拼音。因为将来CA公司的公钥被安装在浏览器或操作系统中后，这个字符串是被显示出来的。等实验完成，你最好删掉CA公司的公钥。那时，你需要用这个字符串来找到它。

为网站创建公钥和私钥：

命令：

```sh
openssl req -nodes -new -keyout site.key -out site.req
```

举例：

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

注意，这一次创建出来的公钥，是用来提交给CA公司进行创建证书的“提交文件”。最后openssl会多问一个问题，让你给“提交文件”设置一个密码，随便设置一个能记得住的即可。

3. 将“提交文件”转换为证书：

首先，创建一个名为“site.ext”的文本文件，并输入如下内容：

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

在本项目的“/ca/localhost”文件夹里，你会找到一个“calc.ext”。可以作为参考。

然后创建证书：

命令：

```sh
openssl x509 -req -CAcreateserial -days 3650 -sha256 -in site.req -CA ../ca.pem -CAkey ../ca.key -extfile site.ext -out site.crt
```

举例：

```sh
agedboy@5R3RZY2:~/dev/ca/issued$ openssl x509 -req -CAcreateserial -days 3650 -sha256 -in site.req -CA ../ca.pem -CAkey ../ca.key -extfile site.ext -out site.crt
Signature ok
subject=CN = localhost
Getting CA Private Key
```

上面命令生成的“site.crt”就是证书了。启动安全网站服务器，你需要的是网站私钥“site.key”，和这个证书“site.crt”。

前面命令生成的“ca.pem”就是CA公司的“公钥”，你需要将它安装到浏览器或者操作系统中。具体安装方法……我真的有点累了，麻烦你自己上网搜吧。比如，搜“如何给火狐（或者Chrome等等）安装CA根证书”。一搜一大堆。

然后就可以用浏览器访问运行在本地实验环境中的HTTPS服务器了。我去，我好累……

如果有什么想聊的，给我发邮件吧。moc.liamg@yobdega