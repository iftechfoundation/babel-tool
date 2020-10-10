# Test babel with Alan (3)

include ../Makefile.boilerplate

.PHONY: $(CURDIR)
$(CURDIR):
	../../babel -ifid valid.a3c > output
	../../babel -ifid invalid.a3c >> output
	$(VERIFY)

clean:
	-rm -f output
