# Test babel basic functionality with various generic files

include ../Makefile.boilerplate

.PHONY: $(CURDIR)
$(CURDIR):
	../../babel -format noifid.bin > output
	../../babel -ifid noifid.bin >> output
	../../babel -format ifid.bin >> output
	../../babel -ifid ifid.bin >> output
	../../babel -format textifid.txt >> output
	../../babel -ifid textifid.txt >> output
	$(VERIFY)

clean:
	-rm -f output
