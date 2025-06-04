include mk/common.mk

CFLAGS += -O2 -std=c99
CFLAGS += -Wall -Wextra

.PHONY: run bootstrap clean

BIN := subleq

all: $(BIN)

$(BIN): subleq.c
	$(VECHO) "  CC+LD\t$@\n"
	$(Q)$(CC) $(CFLAGS) -o $@ subleq.c

run: $(BIN) stage0.dec
	$(Q)./$(BIN) stage0.dec

stage0.dec: subleq.fth
	$(VECHO) "  FORTH\t$@\n"
	$(Q)gforth $< > $@

subleq.fth:
	$(VECHO) "Getting\t$@\n"
	$(Q)wget -q https://raw.githubusercontent.com/howerj/subleq/refs/heads/master/subleq.fth

CHECK_FILES := \
	loops \
	radix \
	bitcount \
	clz \
	log \
	sqrt \
	crc

EXPECTED_loops = *
EXPECTED_radix = 2730
EXPECTED_sqrt = 49

check: $(BIN) stage0.dec
	$(Q)$(foreach e,$(CHECK_FILES),\
	    $(PRINTF) "Running tests/$(e).fth ... "; \
	    if ./$(BIN) stage0.dec < tests/$(e).fth | grep -q "$(strip $(EXPECTED_$(e)))"; then \
	    $(call notice, [OK]); \
	    else \
	    $(PRINTF) "Failed.\n"; \
	    exit 1; \
	    fi; \
	)

# bootstrapping
bootstrap: stage0.dec stage1.dec
	$(Q)if diff stage0.dec stage1.dec; then \
	$(call notice, [OK]); \
	else \
	$(PRINTF) "Unable to bootstrap. Aborting"; \
	exit 1; \
	fi;

stage1.dec: $(BIN) subleq.fth
	$(VECHO)  "Bootstrapping... "
	$(Q)./$(BIN) stage0.dec < subleq.fth > $@

TIME = 5000
TMPDIR := $(shell mktemp -d)
bench: $(BIN) stage0.dec
	$(VECHO)  "Benchmarking... "
	$(Q)(echo "${TIME} ms bye" | time -p ./$(BIN) stage0.dec -s > /dev/null) 2> $(TMPDIR)/bench ; \
	if grep -q real $(TMPDIR)/bench; then \
	$(call notice, [OK]); \
	cat $(TMPDIR)/bench; \
	else \
	$(PRINTF) "Failed.\n"; \
	exit 1; \
	fi;

clean:
	$(RM) $(BIN)

distclean: clean
	$(RM) stage0.dec stage1.dec
	$(RM) subleq.fth
