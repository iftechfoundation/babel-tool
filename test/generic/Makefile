# Test babel basic functionality with various generic files

include ../Makefile.boilerplate

.PHONY: $(CURDIR)
$(CURDIR):
	echo 'noifid.bin' > output
	../../babel -format noifid.bin >> output
	../../babel -ifid noifid.bin >> output
	echo 'ifid.bin' >> output
	../../babel -format ifid.bin >> output
	../../babel -ifid ifid.bin >> output
	echo 'textifid.txt' >> output
	../../babel -format textifid.txt >> output
	../../babel -ifid textifid.txt >> output
	echo 'script.sh' >> output
	../../babel -format script.sh >> output
	../../babel -ifid script.sh >> output
	echo 'scriptifid.sh' >> output
	../../babel -format scriptifid.sh >> output
	../../babel -ifid scriptifid.sh >> output
	echo 'casetest.sh.sh' >> output
	../../babel -format casetest.sh >> output
	../../babel -ifid casetest.sh >> output
	$(VERIFY)

clean:
	-rm -f output
