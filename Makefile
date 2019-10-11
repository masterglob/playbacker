Q=@
BUILD=.build
OUT=out
CC=g++
CFLAGS= -O1 -Wall
LIBS=-lwiringPi
TARGET=$(OUT)/pbkr

default:all
obj :
	@echo "== COMPILE $(FILE)..."
	$(Q)$(CC) $(CFLAGS) -c  -o $(BUILD)/$(FILE).o src/$(FILE)
prepare:
	@echo "== CLEAN... "
	rm -rf $(BUILD) &&  mkdir -p $(BUILD)
link:
	@echo "== LINK... $(TARGET)"
	@mkdir -p $(OUT)
	$(Q)$(CC) -o $(TARGET) $(BUILD)/*.o $(LIBS)
compile:
	@for i in $$(cd src && ls -1 *.c[p]*); do make obj FILE=$$i --no-print-directory || exit 1; done
all:prepare compile link
run : 
	@echo "== RUN... $(TARGET)"
	@$(BUILD)/main.o
