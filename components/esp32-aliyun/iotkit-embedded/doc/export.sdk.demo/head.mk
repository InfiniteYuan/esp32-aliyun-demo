.PHONY: all

CC          := gcc

CFLAGS      := -I../include
CFLAGS      += -I../include/iot-sdk
CFLAGS      += -I../include/mbedtls

LDFLAGS     := -L../lib
LDFLAGS     += -liot_sdk -liot_platform
LDFLAGS     += -lmbedtls -lmbedx509 -lmbedcrypto
LDFLAGS     += -lgcov

