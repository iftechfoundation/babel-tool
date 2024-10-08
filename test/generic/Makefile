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
	../../babel -format script.sh >> output
	../../babel -ifid script.sh >> output
	../../babel -format scriptifid.sh >> output
	../../babel -ifid scriptifid.sh >> output
	$(VERIFY)

clean:
	-rm -f output
