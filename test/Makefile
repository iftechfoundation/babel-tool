all: test-alan test-bronze


.PHONY: test-alan
test-alan:
	$(MAKE) -C alan

.PHONY: test-bronze
test-bronze:
	$(MAKE) -C bronze

clean:
	$(MAKE) -C alan clean
	$(MAKE) -C bronze clean
