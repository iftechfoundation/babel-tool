# Test babel with Adrift

include ../Makefile.boilerplate

.PHONY: $(CURDIR)
$(CURDIR):
	echo Adrift-5: > output
	../../babel -ifid test5.taf >> output
	$(VERIFY)

clean:
	-rm -f output
