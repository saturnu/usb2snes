all: usb2snes

OBJS = usb2snes.c gopt.c

usb2snes: $(OBJS)
	gcc $(OBJS) -o usb2snes

clean:
	rm usb2snes
