OBJ+=main.o
OBJ+=uart_init.o
OBJ+=mqtt.o
OBJ+=msg_link.o

TARGET=cxk
CC=arm-linux-gcc

option=-lmosquitto -lpthread -ldl  -luuid -lssl -lcrypto -I/home/lzx/share/mqtt_arm/mqtt_install/home/edu/share/mqtt_arm/mqtt_install/include -L/home/lzx/share/mqtt_arm/mqtt_install/home/edu/share/mqtt_arm/mqtt_install/lib -L/home/lzx/share/mqtt_arm/ssl_install/lib -L/home/lzx/share/mqtt_arm/uuid_install/lib


$(TARGET) : $(OBJ)
	$(CC) $^ -o $@  $(option)
%.o:%.c
	$(CC) -c $< -o $@ $(option)
.phony:clean
clean:
	rm *.o $(TARGET) -fr