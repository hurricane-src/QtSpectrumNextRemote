# Wifi Uploader for Spectrum Next

## What is it?

It's a small program used to upload .nex programs directly to the RAM of the Spectrum Next.
It's meant to work with https://github.com/hurricane-src/SpectumNextWifiUploader

## How to build

It requires Qt 6 and uses cmake.

```
mkdir build
cd build
cmake
make
```

## How to use
Have the WiFi of your Spectrum Next set to an IP address.
Have wifiupld.nex running

Set the IP address in the field and press "Connect".
Select the .nex you want to upload and press "Send".

The Spectrum Next should print some lines about setting banks.
When all is done, the program should start automatically.
You can then close the program.

## Trouble

If the Spectrum Next doesn't print anything anymore or prints some garbage, it's likely to be the speed is too high for your machine.
The slider can be set to reduce the speed to something supported by your machine.
