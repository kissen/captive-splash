captive-splash
==============

*Annoy people with captive portals!* This is a little program you can
run on a WiFi-enabled ESP82666 micro-controller. It opens a new WiFi
network and then forwards people connecting to the network to a static
captive portal page `payload.html`.


How does it Work?
-----------------

The project comes with a DNS "server" that answers all requests with
the IP address of the ESP8266. Smartphones and browsers detect this as
a captive portal.

Note that I quickly hacked this together, beware of bugs and
incompatibilities.

Building and Running
--------------------

* Get, build and install the [open SDK](https://github.com/pfalcon/esp-open-sdk).

* Install the [esptool](https://github.com/espressif/esptool).

* Update `payload.html` and `user_config.h` to your liking.

* With the ESP8266 connected in programming mode, run `make flash`.

License
-------

I would like this to be GPL, but for now I'm confused about if that's
possible when linking to the proprietary SDK. So for now, I'm not
making definite statement about the license.
