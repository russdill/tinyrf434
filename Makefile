# Config
CONFIG ?= digispark

PROJECT = tinyrf434

CFLAGS =
CONFIGPATH = configs/$(CONFIG)

VUSB_PATH = v-usb
VUSB_INC = $(VUSB_PATH)/usbdrv

VPATH = $(VUSB_INC)

all: targets

CROSS_COMPILE=avr-
CC = $(CROSS_COMPILE)gcc
CPP = $(CROSS_COMPILE)cpp
NM = $(CROSS_COMPILE)nm
OBJCOPY = $(CROSS_COMPILE)objcopy
AVRDUDE = avrdude $(AVRDUDE_OPTS) -p $(DEVICE)

cpp_var = $(shell echo $1 | $(CPP) -I$(VUSB_INC) -I$(CONFIGPATH) -include usbdrv.h -P 2>/dev/null | tail -n 1)

include $(CONFIGPATH)/Makefile.inc

USB_PUBLIC := $(call cpp_var,USB_PUBLIC)

# Options:
CFLAGS +=  -g2 -Os -Wall -Werror
CFLAGS += -I$(CONFIGPATH) -mmcu=$(DEVICE) -DF_CPU=$(F_CPU)
CFLAGS += -fdata-sections -ffunction-sections
CFLAGS += -fno-inline-small-functions -fno-move-loop-invariants -fno-tree-scev-cprop
CFLAGS += -fpack-struct

LDFLAGS += -Wl,--relax

# Common rules
DEPDIR := .d
$(shell mkdir -p $(DEPDIR) >/dev/null)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td
.SUFFIXES:
CLEAN += $(DEPDIR)/*.d $(DEPDIR)/*.Td

config.stamp: FORCE
	@[ -e $@ -a "$$(cat $@ 2>/dev/null)" = "$(CONFIG)" ] || echo -n $(CONFIG) > $@
CLEAN += config.stamp

ALL_DEP = Makefile config.stamp configs/$(CONFIG)/Makefile.inc

.SECONDEXPANSION:
.SECONDARY:

CFLAGS += $(DEPFLAGS) $($@_CFLAGS)
%.o: %.c
%.o: %.c $(DEPDIR)/%.d $(ALL_DEP)
	$(CC) $(CFLAGS) -c $< -o $@
	@mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@

%.o: %.S
%.o: %.S $(DEPDIR)/%.d $(ALL_DEP)
	$(CC) $(CFLAGS) -x assembler-with-cpp -c $< -o $@
	@mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@

LDFLAGS += $($@_LDFLAGS)
%.elf: $(ALL_DEP) $$($$*_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(LDFLAGS)

OBJCOPY_FLAGS += $($@_OBJCOPY_FLAGS)
%.hex: %.elf
%.hex: %.elf $(ALL_DEP)
	$(OBJCOPY) $(OBJCOPY_FLAGS) -j .text -j .data -O ihex $< $@
	@echo Size of sections:
	@avr-size $<
	@echo Size of binary hexfile
	@avr-size $@

%.c: %.xml
%.c: %.xml $(ALL_DEP)
	hidrd-convert $< $@ -i xml -o code

HID_XML=hid.xml
HID_C=$(HID_XML:.xml=.c)

# Build output
$(PROJECT)_OBJECTS = main.o usbdrvasm.o
ifeq ($(USB_PUBLIC),static)
main.o_CFLAGS += -include usbdrv.c
else
$(PROJECT)_OBJECTS += usbdrv.o
endif
main.o: $(HID_C)
$(PROJECT).elf: CFLAGS += -DUSB_CFG_HID_REPORT_DESCRIPTOR_LENGTH=$(shell hidrd-convert $(HID_XML) -i xml | wc -c)
$(PROJECT).elf: CFLAGS += -DHID_C=\"$(HID_C)\"
$(PROJECT).elf: CFLAGS += -I$(VUSB_INC)

TARGETS += $(PROJECT).hex
CLEAN += $(PROJECT).hex $(PROJECT).elf $(HID_C) $($(PROJECT)_OBJECTS)

flash: $(PROJECT).hex
	$(AVRDUDE) -U $<

# Common end file targets
targets: $(TARGETS)

clean: FORCE
	@rm -f $(CLEAN)
	@rmdir --ignore-fail-on-non-empty $(DEPDIR)

.PHONY: $(PHONY)
FORCE:

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

include $(wildcard $(DEPDIR)/*.d)

