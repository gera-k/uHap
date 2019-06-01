# uHap - Homekit Accessory Protocol server

The HAP server implements HomeKit Accessory Protocol for IP Accessories as defined by the HomeKit Accessory Protocol Specification (Non-Commertial version) Release R1.

The core module is designed to be small and portable. Currently its only dependency is standard C++ library.

Source code structure:
* __Hap__ - core module
* __Crypto__ - Cryptography
* __CryptoTest__ - Cryptography test
* __Linux__ - Linux platform specifics
* __Windows__ - Windows platform specifics
* __Util__
* __WinTest__ - Windows test server
* __RpiTest__ - Linux(Raspbian)-based light accessory

## Building for Windows

* Visual Studio 2019 Preview is required.
* The project uses Apple Bojour SDK. Install it and create BONJOUR_SDK environment variable pointing to the SDK root dir. Example:
```
  >echo %BONJOUR_SDK%
  C:\Program Files\Bonjour SDK
```
* Open the uHap.sln, set the WinTest project as starup, and build it.

## Building for Raspbian

* Build hardware. Refer to [CuteLight](https://www.hackster.io/gera_k/homekit-controlled-multicolor-light-36279b) project on Hackster for details.
* Setup RPi for WiFi 
* Login to RPi 
* Install Bonjour compatibility library:
```
sudo apt install libavahi-compat-libdnssd-dev
```
* Clone, build, and install the RpiHap application:
```
pi@raspberrypi:~/github $ git clone https://github.com/gera-k/uHap.git
Cloning into 'uHap'...
remote: Enumerating objects: 91, done.
remote: Counting objects: 100% (91/91), done.
remote: Compressing objects: 100% (71/71), done.
remote: Total 91 (delta 18), reused 87 (delta 17), pack-reused 0
Unpacking objects: 100% (91/91), done.
pi@raspberrypi:~/github $ cd uHap/RpiHap/
pi@raspberrypi:~/github/uHap/RpiHap $ make
Generating dependencies...
gcc -O2 -I. -I.. -Wall -c ../Hap/picohttpparser.c -o picohttpparser.o
gcc -O2 -I. -I.. -Wall -c ../Hap/Hap.cpp -o Hap.o
gcc -O2 -I. -I.. -Wall -c ../Hap/HapDb.cpp -o HapDb.o
gcc -O2 -I. -I.. -Wall -c ../Hap/HapHttp.cpp -o HapHttp.o
gcc -O2 -I. -I.. -Wall -c ../Hap/jsmn.cpp -o jsmn.o
gcc -O2 -I. -I.. -Wall -c ../Crypto/Aead.cpp -o Aead.o
gcc -O2 -I. -I.. -Wall -c ../Crypto/Chacha20.cpp -o Chacha20.o
gcc -O2 -I. -I.. -Wall -c ../Crypto/Curve25519.cpp -o Curve25519.o
gcc -O2 -I. -I.. -Wall -c ../Crypto/Ed25519.cpp -o Ed25519.o
gcc -O2 -I. -I.. -Wall -c ../Crypto/HmacSha512.cpp -o HmacSha512.o
gcc -O2 -I. -I.. -Wall -c ../Crypto/MD.cpp -o MD.o
gcc -O2 -I. -I.. -Wall -c ../Crypto/Poly1305.cpp -o Poly1305.o
gcc -O2 -I. -I.. -Wall -c ../Crypto/Sha512.cpp -o Sha512.o
gcc -O2 -I. -I.. -Wall -c ../Crypto/Sha512blk.cpp -o Sha512blk.o
gcc -O2 -I. -I.. -Wall -c ../Crypto/Srp.cpp -o Srp.o
gcc -O2 -I. -I.. -Wall -c ../CryptoTest/Aeadtest.cpp -o Aeadtest.o
gcc -O2 -I. -I.. -Wall -c ../CryptoTest/Chacha20test.cpp -o Chacha20test.o
gcc -O2 -I. -I.. -Wall -c ../CryptoTest/CryptoTest.cpp -o CryptoTest.o
gcc -O2 -I. -I.. -Wall -c ../CryptoTest/Curve25519test.cpp -o Curve25519test.o
gcc -O2 -I. -I.. -Wall -c ../CryptoTest/Ed25519test.cpp -o Ed25519test.o
gcc -O2 -I. -I.. -Wall -c ../CryptoTest/HkdfSha512test.cpp -o HkdfSha512test.o
gcc -O2 -I. -I.. -Wall -c ../CryptoTest/HmacSha512test.cpp -o HmacSha512test.o
gcc -O2 -I. -I.. -Wall -c ../CryptoTest/MdTest.cpp -o MdTest.o
gcc -O2 -I. -I.. -Wall -c ../CryptoTest/Poly1305test.cpp -o Poly1305test.o
gcc -O2 -I. -I.. -Wall -c ../CryptoTest/Sha512test.cpp -o Sha512test.o
gcc -O2 -I. -I.. -Wall -c ../CryptoTest/SrpTest.cpp -o SrpTest.o
gcc -O2 -I. -I.. -Wall -c ../Util/FileLog.cpp -o FileLog.o
gcc -O2 -I. -I.. -Wall -c ../Linux/HapMdns.cpp -o HapMdns.o
gcc -O2 -I. -I.. -Wall -c ../Linux/HapTcp.cpp -o HapTcp.o
gcc -O2 -I. -I.. -Wall -c RpiHap.cpp -o RpiHap.o
gcc  picohttpparser.o Hap.o HapDb.o HapHttp.o jsmn.o Aead.o Chacha20.o Curve25519.o Ed25519.o HmacSha512.o MD.o Poly1305.o Sha512.o Sha512blk.o Srp.o Aeadtest.o Chacha20test.o CryptoTest.o Curve25519test.o Ed25519test.o HkdfSha512test.o HmacSha512test.o MdTest.o Poly1305test.o Sha512test.o SrpTest.o FileLog.o HapMdns.o HapTcp.o RpiHap.o  -lm -lstdc++ -lpthread -ldns_sd -lwiringPi -o RpiHap
pi@raspberrypi:~/github/uHap/RpiHap $ sudo make install
install RpiHap /usr/sbin
install homekit /etc/init.d
pi@raspberrypi:~/github/uHap/RpiHap $ sudo systemctl enable homekit
homekit.service is not a native service, redirecting to systemd-sysv-install.
Executing: /lib/systemd/systemd-sysv-install enable homekit
pi@raspberrypi:~/github/uHap/RpiHap $ sudo systemctl start homekit
pi@raspberrypi:~/github/uHap/RpiHap $ ps ax | grep Rpi
 2497 ?        Sl     0:00 /usr/sbin/RpiHap
 2508 pts/0    S+     0:00 grep --color=auto Rpi
pi@raspberrypi:~/github/uHap/RpiHap $  
```
