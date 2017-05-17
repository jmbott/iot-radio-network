# iot-radio-network
Wireless point to point radio network

http://iotcolumbia2017jsjc.weebly.com/

## Setup

For compile,

* Arduino IDE (v1.8.2 tested on Linux)
* Need M0 board files (Arduino SAMD core)

* Ethernet2 library
* lightOS library
* Arduino SPI library
* RadioHead library
* Timer library

## Overview

Both controllers use interrupts through the lightOS and Timer libraries allowing 
for asynchronous behavior.

**controller**

Contains code for main controller. Manages web server, requests and stores meter
values in memory. Transfers request for status change to selected meter.

**remote_node**

Contains code for remote nodes. Polls meter(s) and stores information in memory.
Listens for commands from main controller, responds if request is applicable to
self.

**/docs**

Contains transmission schema for RF and meter communications.

**/archive**

Contains implemented and unimplemented code for testing functions.
