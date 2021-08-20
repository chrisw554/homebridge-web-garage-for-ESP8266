# homebridge-web-garage-for-ESP8266
ESP8266 code to work with the homebridge-web-garage plugin found here: https://github.com/Tommrodrigues/homebridge-web-garage

If you're looking for something which doesn't rely on homebridge then I recommend this one, I'm not using it due to my own Wifi issues: https://github.com/jmbwell/GarageDoorOpener

You'll need an ESP8266 board such as D1 Mini, a Relay or Relay Shield and two contact/magnetic reed sensors/switches. You'll need to work out how to mount your reed switches (one for open, one for closed) and how to trigger your garage door opener with the relay.

Update your wifi details, PIN numbers and Homebridge URL, Upload the code and you're good to go.

Make a note of the IP printed to serial monitor on booting to input when adding the garage door opener to Homebridge.

Setting up a DHCP MAC Address reservation on your router is recommended since the plugin crashes homebridge if the ESP8266 doesn't respond.
