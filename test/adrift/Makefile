# Test babel with Adrift

include ../Makefile.boilerplate

.PHONY: $(CURDIR)
$(CURDIR):
	echo Adrift-5: > output
	../../babel -ifid test5.taf >> output
	../../babel -identify test5.taf >> output
	../../babel -ifiction test5.taf >> output
	diff ADRIFT-500-8FAD-A170-4086-B29E-C46FEED294AA.iFiction expected.iFiction >> output
	echo Adrift-4: >> output
	../../babel -ifid test4.taf >> output
	../../babel -identify test4.taf >> output
	../../babel -ifiction test4.taf >> output
	$(VERIFY)

clean:
	-rm -f output ADRIFT-*
