Q=@
BUILD=.build
OUT=out
CC=g++
CFLAGS= -O1 -Wall
LIBS=-lwiringPi
LIBS+=-lasound
LIBS+=-lpthread


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
i2s:prepare
	$(Q)make obj FILE=main_i2s.cpp --no-print-directory
	$(Q)make obj FILE=pbkr_snd.cpp --no-print-directory
	$(Q)make obj FILE=pbkr_utils.cpp --no-print-directory
	$(Q)make link TARGET=i2s --no-print-directory 
pbkr:prepare
	$(Q)make obj FILE=main_pbkr.cpp --no-print-directory &
	$(Q)make obj FILE=pbkr_gpio.cpp --no-print-directory & 
	$(Q)make obj FILE=pbkr_wav.cpp --no-print-directory
	$(Q)make obj FILE=pbkr_display.cpp --no-print-directory
	$(Q)make obj FILE=pbkr_snd.cpp --no-print-directory
	$(Q)make obj FILE=pbkr_utils.cpp --no-print-directory
	$(Q)make link TARGET=pbkr --no-print-directory 
clean:
	rm -f src/* .build/*