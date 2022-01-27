
# Reported issues

Hi, I did the cutting trick on a Wemos clone (I didn't change the ESP, so standard ESP 12F). Now I'm only able to make it work if the USB cable is plugged, if on battery not. I attach a link to the picture of the PCB, which is slightly different. I checked voltages and even with only the battery they're ok.... I can't understand... https://drive.google.com/drive/folders/1bRNhNGyD8aAU0ezcsKt2C6N1GyzYppOX?usp=sharing

I found the solution by myself. After cutting the track D3 was no longer able to power bme280, so no output on MQTT. I had just a few time and I verified that powering BME280 with 3V3 is ok, tomorrow I'll try add something to power the sensor as the Wemos is on to save energy

---

I want to point out some clarification regarding the cutting of the PCB trace:
I did as instructed in the video. However i got strange behavoiur, as the ESP did not work properly afterwards. After some time debugging I found that by cutting the trace as suggested, the Pull-ups for D3 and D4 have no 3V3 anymore. So I connected the pull ups with a short wire to 3V3. But I had to cut the trace which leads to Pin16 of the CH340 to prevent the Chip from getting power again.
I han further problems with the 470R Resistors in the RX and TX line. I found that somehow current was flowing from ESP to CH340 through them. Therefore i desoldered them. Now I can only program via external serial converter, but it's working now as expected.

BTW: my blue LED was also slighly glowing during deep sleep as you described. I suspect that this is caused by the above mentioned issue. I suspect in your case (only the one trace cut) current flows from GPIO2 (where LED is) throught the circuit path (the disconnected 3V3 copper traces) and through the CH340 chip back to the ESP. Probably due to some parasitic diodes. With my additional modification (cut 2nd trace and solder a short wire), the LED does not glow anymore during deep sleep.

# Ideas

The LDO of the Wemos is really not that bad: If you connect a 3.7V 18650 cell directly to the 5V and GND pin of a Wemos D1 Mini, it draws around 0.15-0.2mA in deepsleep, which quiet good for an LDO. At least that's what i measured. I use a Wemos D1 with one 18650 LiPo for a temperature and humidity sensor (BME280) and it lasts for around six month with one charge. The wake interval is five minutes.

---

