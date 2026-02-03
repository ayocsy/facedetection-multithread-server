
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=gnu99
LIBS = -L/usr/lib64 -lopencv_core -lopencv_imgcodecs -lopencv_objdetect -lopencv_imgproc -lpthread

CLIENT_OBJECTS = uqfaceclient.o protocol.o
DETECT_OBJECTS = uqfacedetect.o protocol.o

all: uqfaceclient uqfacedetect

uqfaceclient: $(CLIENT_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(CLIENT_OBJECTS)

uqfacedetect: $(DETECT_OBJECTS)
	$(CC) $(CFLAGS) $(LIBS) -o $@ $(DETECT_OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) $(LIBS) -c $<

clean:
	rm -f *.o uqfaceclient uqfacedetect

.PHONY: all clean
