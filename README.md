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

## How to use the GUI
Have the WiFi of your Spectrum Next set to an IP address.
Have wifiupld.nex running

Set the IP address in the field and press "Connect".
Select the .nex you want to upload and press "Send".

The Spectrum Next should print some lines about setting banks.
When all is done, the program should start automatically.
You can then close the program.

## How to use the command line
```
./QtSpectrumNextRemoteCLI --host spectrumnext_address --file /path/to/executable.nex [--rate bytes/s]
```
e.g.
```
./QtSpectrumNextRemoteCLI --host 192.168.1.70  --file /home/login/Downloads/executable.nex --rate 8192
```

Using --help works too.

## Trouble

If the Spectrum Next doesn't print anything anymore or prints some garbage, it's likely to be the speed is too high for your machine.
The slider can be set to reduce the speed to something supported by your machine.

## Example file

I've made all my tests with:
https://github.com/hurricane-src/Files/raw/main/shtrtst.nex

That program is another Spectrum Next project.
* The music has been taken from a bunch of Atari ones.
* The backround is a modified image originally taken from the NASA.
* The sprites are assets bought on itch.io (Foozle)
* The tileset is from yours truly (Yes, it's ugly).
* Keys are Q,A,O,P,SPACE

It may only work on 2M machines though as it's kind of big.

