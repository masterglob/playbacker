Q=@
BUILD=.build
OUT=out
CC=g++
CFLAGS= -O1 -Wall -I/usr/local/include/libxml2
LIBS=-lwiringPi
LIBS+=-lasound
LIBS+=-lpthread
LIBS+=-lxml2


default:all
obj :
	@echo "== COMPILE $(FILE)..."
	$(Q)$(CC) $(CFLAGS) -c  -o $(BUILD)/$(FILE).o src/$(FILE) -Isrc
prepare:
	@echo "== CLEAN... "
	rm -rf $(BUILD) &&  mkdir -p $(BUILD)
link:
	@echo "== LINK... $(OUT)/$(TARGET)"
	@mkdir -p $(OUT)
	$(Q)$(CC) -o $(OUT)/$(TARGET) $(BUILD)/pbkr*.o $(BUILD)/main_$(TARGET).* $(LIBS)
link_pbkr:
	$(Q)make link TARGET=pbkr --no-print-directory
link_display:
	$(Q)make link TARGET=lcd_display --no-print-directory
compile:
	@for i in $$(cd src && ls -1 *.c[p]*); do make obj FILE=$$i --no-print-directory || exit 1; done
all:prepare compile link_pbkr
test:
	$(Q)make obj FILE=main_test.cpp --no-print-directory 
	$(Q)make link TARGET=test --no-print-directory 
main:
	$(Q)make obj FILE=main_pbkr.cpp --no-print-directory 
	$(Q)make link TARGET=pbkr --no-print-directory 
midi:
	$(Q)make obj FILE=pbkr_midi.cpp --no-print-directory 
	$(Q)make link TARGET=pbkr --no-print-directory 
osc:
	$(Q)make obj FILE=pbkr_osc.cpp --no-print-directory 
	$(Q)make link TARGET=pbkr --no-print-directory 
utils:
	$(Q)make obj FILE=pbkr_utils.cpp --no-print-directory
	$(Q)make link TARGET=pbkr --no-print-directory 
web:
	$(Q)make obj FILE=pbkr_websrv.cpp --no-print-directory 
	$(Q)make link TARGET=pbkr --no-print-directory 
wav:
	$(Q)make obj FILE=pbkr_wav.cpp --no-print-directory 
	$(Q)make link TARGET=pbkr --no-print-directory 
display:
	$(Q)make obj FILE=pbkr_display.cpp --no-print-directory 
	$(Q)make link TARGET=pbkr --no-print-directory 
pbkr:prepare
	$(Q)make obj FILE=main_pbkr.cpp --no-print-directory &
	$(Q)make obj FILE=pbkr_gpio.cpp --no-print-directory & 
	$(Q)make obj FILE=pbkr_midi.cpp --no-print-directory &
	$(Q)make obj FILE=pbkr_wav.cpp --no-print-directory &
	$(Q)make obj FILE=pbkr_websrv.cpp --no-print-directory 
	$(Q)make obj FILE=pbkr_osc.cpp --no-print-directory 
	$(Q)make obj FILE=pbkr_display.cpp --no-print-directory 
	$(Q)make obj FILE=pbkr_snd.cpp --no-print-directory
	$(Q)make obj FILE=pbkr_utils.cpp --no-print-directory
	$(Q)make link TARGET=pbkr --no-print-directory 
clean:
	rm -f src/* .build/*