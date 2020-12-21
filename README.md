# secretsanta2020 :santa:

This is a project for Rian who was on Santa's good-list.
Santa got his name through the Secret Santa 2020 campaign of the [MakeInBelgium facebook group](https://www.facebook.com/groups/makeinbelgium/permalink/3414757655276470).

I made him a digital LED segment clock with his name in modular 3D printed enclosures. The digits have a uniform color. But the Rian-logo and the `:` in the middle color cycle slowly.

![SecretSantaClock](https://lh3.googleusercontent.com/pw/ACtC-3eVZhOMHTfICzi0teHSat15XgUNSiLqqCiNRee5lyfhbFXGZaFYJ4gO0QFXKAGCdoFwdbEP7DYLsP22yx-xGMuc261i1WjAGfB3t8Bz2P4to3SfvMSJvvKibINTjG5FJ4ud8s5aqXQ7LxhhNMFakMyr=w1080-h810-no?authuser=0)

![Wireframe](https://lh3.googleusercontent.com/pw/ACtC-3fIov0i_BTv4j1BN9GwLmHVfTGJo_IWrwm7Qs_9xoxIL6i_s5zvjfWdPwjXccUIdcjyego-vnKVyUJALI_zz-KxH6KLB7czvyYNXrcke569lO0McK-JgmqaxKOjp_Wfl9x5aJYRNcLvwjpR3PBZJXpc=w963-h810-no)

[Photo gallery](https://photos.app.goo.gl/esswfGq329zNh7Nb6)

## Offline features

It works plug-and-play. Just attach the micro-usb cable to a 5V power supply and you should see the time. The device contains a RTC and the time is set to CET.

There is also a light sensor that adjusts the brightness of the clock to the ambiant light level. So during the day it will be brighter and in the evening when the room is darker, it will shine less bright.

But offline is kinda boring, so ... i got it connected.

## Configuration

When plugged in, it starts an access-point named `SecretSantaAP`. Once you connect to it, you should be sent to the config-page automatically as a captive portal. If you are not redirected you can browse to `http://192.168.4.1` manually.

![Configuration](https://lh3.googleusercontent.com/pw/ACtC-3fxRZ9qJlxG3YoOpsDI95kS690p7wfzn1DI769k2gMZL0IllzKtDfk-TMJoJpl_HCifMcWCNolkzsp2s_H0nWb3tcXFJUJKLFxhQGRke-2RL0HxttUtZxFtF-XCdNrAAulKLNfGe6EFgQELGIl7BNxT=w931-h607-no?authuser=0)

You can select your wifi accesspoint from the list and enter your password.

There are also fields to connect to a MQTT server.
* servername
* port
* username
* password
* state topic
* set topic

It also has a field to change the NTP server pool. You can leave it to the default `europe.pool.ntp.org`.

The field `color 1` is the primary color used for the digits of the clock. The rgb values are set in this format : `255,0,0` (This gives red).

Other values are not used for the moment.

On the back there is a small push button, push it for 2 seconds with a pen to reset all configurations. The clock will light up white and takes about 10 seconds to reset and clear all configuration.

## Connected features

When connected to your network and the internet. The clock will sync with an NTP sever regulary. It will auto adjust to CET daylight saving time.

If you have a home automation system that talks MQTT you can turn off the clock when you are away from home or at night.
Just send a json message to the default topic `secretsanta/clock/set` (or the topic you have configured) with the following payload:

```json
{"mode":"OFF"}
```

To turn back on, send

```json
{"mode":"CLOCK"}
```

## Just the start

Of course this is just the start, that is why i am sharing the code. Consider it a startingpoint. The fundaments are there, now you can extend it as you wish.
I have a beta version of this clock and it shows the indoor and outdoor temperature in different colors. It shows the progress % of my 3D printer ... 
I am not a C++ programmer so sorry for my bad coding, i noticed there is still some memoryleak in there, but hey, it kinda works :-)

## Credits

I got the initial idea (for my beta clock) from [Ivan Miranda](https://ivanmiranda.com) who posted a video on his youtube channel making a basic version of this clock. So thanks to Ivan for his sharing his work !

[![Watch the video](https://img.youtube.com/vi/PixXKK8N_wA/hqdefault.jpg)](https://youtu.be/PixXKK8N_wA)

## Main components

* Black and White PLA
* Creality CR6-SE 3D printer
* ws2812b 60 leds/m led-strips (100 leds used in this clock)
* ESP2866 module (Wemos D1 mini)
* DS3232 Real time clock module
* male and female micro-usb connector for connecting the ESP to the enclosure
* G5516 photo resistor
* lots of hot-glue