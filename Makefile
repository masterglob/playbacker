Q=@
BUILD=.build
OUT=out
CC=g++
CFLAGS= -O1 -Wall
LIBS=-lwiringPi


default:all
obj :
	@echo "== COMPILE $(FILE)..."
	$(Q)$(CC) $(CFLAGS) -c  -o $(BUILD)/$(FILE).o src/$(FILE)
prepare:
	@echo "== CLEAN... "
	rm -rf $(BUILD) &&  mkdir -p $(BUILD)
link:
	@echo "== LINK... $(OUT)/$(TARGET)"
	@mkdir -p $(OUT)
	$(Q)$(CC) -o $(OUT)/$(TARGET) $(BUILD)/pbkr*.o $(BUILD)/main_$(TARGET).* $(LIBS)
link_pbkr:
	$(Q)make link TARGET=pbkr
link_display:
	$(Q)make link TARGET=lcd_display
compile:
	@for i in $$(cd src && ls -1 *.c[p]*); do make obj FILE=$$i --no-print-directory || exit 1; done
all:prepare compile link_pbkr
clean:
	rm -f src/* .build/*