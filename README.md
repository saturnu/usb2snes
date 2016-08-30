# usb2snes
dev usb rom/sram upload for the sd2snes

#### linux ####

Build:
make

Examples:

- Upload rom + sram - ttyACM0
./usb2snes --device=/dev/ttyACM0 --sram --write --file=myrom.srm
./usb2snes --device=/dev/ttyACM0 --write --file=myrom.smc
./usb2snes --device=/dev/ttyACM0 --boot



#### Windows ####

Build:
make -f Makefile.win32

Examples:

- Upload rom + sram - COM4
./usb2snes --device=4 --sram --write --file=myrom.srm
./usb2snes --device=4 --write --file=myrom.smc
./usb2snes --device=4 --boot


Hint:
some games have an additional 512byte backup unit header,
use the --skip option here


greetings, saturnu
