# Test babel basic functionality with various generic files

include ../Makefile.boilerplate

.PHONY: $(CURDIR)
$(CURDIR):
	../../babel -format noifid.bin > output
	../../babel -ifid noifid.bin >> output
	$(VERIFY)

clean:
	-rm -f output
