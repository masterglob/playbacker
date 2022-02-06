Q=@
BUILD=.build
OUT=out
CC=g++
CFLAGS= -O1 -Wall -I/usr/local/include/libxml2 -g
# CFLAG += -g
LIBS=-lwiringPi 
LIBS+=-lasound
LIBS+=-lpthread
# LIBS+=-lxml2
LIBS +=  -g

LCD_OBJS=$(BUILD)/pbkr_display.cpp.o $(BUILD)/pbkr_gpio.cpp.o
default:all
obj :
	@echo "== COMPILE $(FILE)..."
	$(Q)$(CC) $(CFLAGS) -DBUILD_ID=$(BUILD_ID) -c  -o $(BUILD)/$(FILE).o src/$(FILE) -Isrc
prepare:
	@echo "== CLEAN... "
	rm -rf $(BUILD) &&  mkdir -p $(BUILD) && chmod -R 777  $(BUILD)
link:
	@echo "== BUILD ID=$(BUILD_ID)"
	$(Q)make obj FILE=pbkr_version.cpp --no-print-directory 
	@echo "== LINK... $(OUT)/$(TARGET)"
	@mkdir -p $(OUT)
	$(Q)$(CC) -o $(OUT)/$(TARGET) $(BUILD)/pbkr*.o $(BUILD)/main_$(TARGET).* $(LIBS)
link_pbkr:
	$(Q)make link TARGET=pbkr --no-print-directory
link_display:
	@echo "== LINK... $(OUT)/lcd_display"
	@mkdir -p $(OUT)
	$(Q)$(CC) -o $(OUT)/lcd_display $(LCD_OBJS) $(BUILD)/main_lcd_display.* $(LIBS)
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
snd:
	$(Q)make obj FILE=pbkr_snd.cpp --no-print-directory
	$(Q)make link TARGET=pbkr --no-print-directory 
menu:
	$(Q)make obj FILE=pbkr_menu.cpp --no-print-directory 
	$(Q)make link TARGET=pbkr --no-print-directory 
gpio:
	$(Q)make obj FILE=pbkr_gpio.cpp --no-print-directory 
	$(Q)make link TARGET=pbkr --no-print-directory 
projects:
	$(Q)make obj FILE=pbkr_projects.cpp --no-print-directory 
	$(Q)make link TARGET=pbkr --no-print-directory 
wav:
	$(Q)make obj FILE=pbkr_wav.cpp --no-print-directory 
	$(Q)make link TARGET=pbkr --no-print-directory 
display:
	$(Q)make obj FILE=pbkr_display.cpp --no-print-directory 
	$(Q)make obj FILE=pbkr_display_mgr.cpp --no-print-directory 
	$(Q)make link TARGET=pbkr --no-print-directory 
console:
	$(Q)make obj FILE=pbkr_console.cpp --no-print-directory 
	$(Q)make link TARGET=pbkr --no-print-directory 
cfg:
	$(Q)make obj FILE=pbkr_cfg.cpp --no-print-directory 
	$(Q)make link TARGET=pbkr --no-print-directory 
api:
	$(Q)make obj FILE=pbkr_api.cpp --no-print-directory 
	$(Q)make link TARGET=pbkr --no-print-directory 
websrv:
	$(Q)make obj FILE=pbkr_websrv.cpp --no-print-directory 
	$(Q)make link TARGET=pbkr --no-print-directory 
lcd_display:
	$(Q)make obj FILE=pbkr_gpio.cpp --no-print-directory &
	$(Q)make obj FILE=pbkr_display.cpp --no-print-directory 
	$(Q)make obj FILE=main_lcd_display.cpp --no-print-directory
	$(Q)make link_display --no-print-directory
pbkr:prepare
	$(Q)make obj FILE=main_pbkr.cpp --no-print-directory &
	$(Q)make obj FILE=pbkr_gpio.cpp --no-print-directory
	$(Q)make obj FILE=pbkr_midi.cpp --no-print-directory & 
	$(Q)make obj FILE=pbkr_wav.cpp --no-print-directory 
	$(Q)make obj FILE=pbkr_console.cpp --no-print-directory &
	$(Q)make obj FILE=pbkr_websrv.cpp --no-print-directory 
	$(Q)make obj FILE=pbkr_osc.cpp --no-print-directory &
	$(Q)make obj FILE=pbkr_display.cpp --no-print-directory
	$(Q)make obj FILE=pbkr_display_mgr.cpp --no-print-directory &
	$(Q)make obj FILE=pbkr_cfg.cpp --no-print-directory 
	$(Q)make obj FILE=pbkr_menu.cpp --no-print-directory &
	$(Q)make obj FILE=pbkr_snd.cpp --no-print-directory
	$(Q)make obj FILE=pbkr_api.cpp --no-print-directory &
	$(Q)make obj FILE=pbkr_utils.cpp --no-print-directory
	$(Q)make obj FILE=pbkr_projects.cpp --no-print-directory
	$(Q)make link TARGET=pbkr --no-print-directory 
clean:
	rm -f src/* .build/*