# secretsanta2020 :santa:

This is a project for a boy called Rian who is on Santa's good-list.
Santa got his name through the Secret Santa 2020 campaign of the MakeInBelgium facebrook group. () https://www.facebook.com/groups/makeinbelgium/permalink/3414757655276470 )

This is a digital LED segment clock with a logo in modular 3D printed enclosures.

## Offline features

It works plug-and-play. Just attach the micro-usb cable to a 5V power supply and you should see the time. The device contains a RTC and the time is set to CET.

There is also a light sensor that adjusts the brightness of the clock to the ambiant light level. So during the day it will be brighter and in the evening when the room is darker, it will shine less bright.

But that would be kinda boring, so ... i got it connected.

## Configuration

When plugged in, it starts an access-point named `SecretSantaAP`. It should send you to the configpage automatically as a captive portal. If you are not redirected you can browse to `http://192.168.4.1` manually.

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

On the back there is a small push button

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
I have a beta version of this clock and it shows the indoor and outdoor temperature in different colors. It shows the progress of my 3D printer ... 

## Credits

I got the initial idea (for my beta clock) from [Ivan Miranda](https://ivanmiranda.com) who posted a video on his youtube channel making a basic version of this clock. So thanks to Ivan for his sharing his work !

<iframe width="560" height="315" src="https://www.youtube.com/embed/PixXKK8N_wA" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>

## Main components

* Black and White PLA
* Creality CR6-SE 3D printer
* ws2812b 60 leds/m led-strips (100 leds used in this clock)
* ESP2866 module (Wemos D1 mini)
* DS3232 Real time clock module
* male and female micro-usb connector for connecting the ESP to the enclosure
* G5516 photo resistor
* lots of hot-glue