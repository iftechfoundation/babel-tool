all: test-adrift test-alan test-hugo test-twine test-html test-bronze test-sensory test-generic


.PHONY: test-adrift
test-adrift:
	$(MAKE) -C adrift

.PHONY: test-alan
test-alan:
	$(MAKE) -C alan

.PHONY: test-hugo
test-hugo:
	$(MAKE) -C hugo

.PHONY: test-twine
test-twine:
	$(MAKE) -C twine

.PHONY: test-html
test-html:
	$(MAKE) -C html

.PHONY: test-bronze
test-bronze:
	$(MAKE) -C bronze

.PHONY: test-sensory
test-sensory:
	$(MAKE) -C sensory

.PHONY: test-generic
test-generic:
	$(MAKE) -C generic

clean:
	$(MAKE) -C adrift clean
	$(MAKE) -C alan clean
	$(MAKE) -C hugo clean
	$(MAKE) -C twine clean
	$(MAKE) -C html clean
	$(MAKE) -C bronze clean
	$(MAKE) -C sensory clean
	$(MAKE) -C generic clean
