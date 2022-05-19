# NDI Recorder

NDI Recorder is an application that connects to any Full NDI stream and records the stream to storage.

## Features
- Manage NDI recording sources using the integrated web server
- Record multiple NDI streams at one time
- Uses the latest version of NDI - NDI 5

## Supported devices

This software is tested with a Raspberry Pi 4 (32 and 64-bit). 

### Download required installation files

Make sure git is installed.

```
sudo apt update
sudo apt install git
```
Clone this repository and `cd` into it.

```
git clone https://github.com/lplassman/NDI-Recorder.git && cd NDI-Recorder
```

### Install on Raspberry Pi 4 64-bit

Run this compile and install script

```
sudo bash ./easy-install-rpi4-aarch64.sh
```
Installation is now complete!


### Install on Raspberry Pi 4 32-bit

Run this compile and install script

```
sudo bash ./easy-install-rpi4-armhf.sh
```
Installation is now complete!


### Install on Raspberry Pi 3 32-bit

Run this compile and install script

```
sudo bash ./easy-install-rpi3-armhf.sh
```
Installation is now complete!



### Install on x86_64 bit (Intel/AMD)

Run this compile and install script

```
sudo bash ./easy-install-x86_64.sh
```
Installation is now complete!


### Install on generic ARM64

Compiling on generic ARM64 requires use of the NDI Advanced SDK. Due to licensing restrictions, the NDI Advanced SDK must be downloaded manually from NDI's website: ndi.tv
Extract the downloaded NDI Advanced SDK .tar file and copy it to the NDI-Recorder directory on the target device. This can be achieved by using FTP, SCP, or Samba.

Compile and install

```
sudo bash ./easy-install-generic-aarch64.sh
```
Installation is now complete!


## Usage for NDI Recorder

Once the installation process is complete, it will create an executable file located at /opt/ndi_recorder/bin/ndi_recorder

The installer also creates a symlink to /usr/bin so that it can be run from a normal terminal.

To run and start the web server while specifying a path to where the recordings should be stored:

```
sudo ndi_recorder -p /media/storage/
```

## Install service file for starting ndi_recorder on boot

By default this service file runs ndi_recorder as the root user. Customize the save path before installing:

```
sudo nano ./ndi_recorder.service
```
Install the service file

```
sudo cp ./ndi_recorder.service /etc/systemd/system/
sudo systemctl enable ndi_recorder.service
sudo systemctl start ndi_recorder.service
```
