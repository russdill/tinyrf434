F_CPU := $(call cpp_var,F_CPU)
DEVICE := $(call cpp_var,DEVICE)

MEIOSIS_PATH=~/src/vmeiosis

VMEDUDE = $(MEIOSIS_PATH)/scripts/vmedude.py $(VMEDUDE_OPTS)

vme_flash: tws-bs-hid.hex
	$(VMEDUDE) --enter --product RF --id-product 05df || true
	$(VMEDUDE) -U $< --run
