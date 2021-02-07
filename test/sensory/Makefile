# Test babel basic functionality with sensory.blb

include ../Makefile.boilerplate

.PHONY: $(CURDIR)
$(CURDIR):
	../../babel -ifid sensory.blb > output
	../../babel -format sensory.blb >> output
	../../babel -identify sensory.blb >> output
	$(VERIFY)

clean:
	-rm -f output
