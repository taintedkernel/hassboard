# smartgirder
Home Assistant-based RGB LED matrix display

This is my version of a smart display, inspired by and initially based off [smartbutnot's project](https://github.com/smartbutnot/smartgirder), but with some design changes.  I'm utilizing a Raspberry Pi Zero 2 as the controller for mine, along with Adafruit's [RGB Matrix Bonnet](https://www.adafruit.com/product/3211).  My first attempts to get the image quality I was looking for was not possible with the Matrix Portal that smartbutnot leveraged, hence the bonnet.  Other obvious differences include the code being developed as a Linux application, rather then an Arduino firmware.

My display is focused primarily on outdoor weather and internal sensors in multiple rooms, while also including data such as upcoming calendar events, weather alerts and general reminders.  It is also MQTT and HomeAssistant-based, thus relying heavily on HA automations in order to obtain the necessary data.
