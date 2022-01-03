# Test babel with HTML

include ../Makefile.boilerplate

.PHONY: $(CURDIR)
$(CURDIR):
	../../babel -format Test-Game.html > output
	../../babel -ifid Test-Game.html >> output
	../../babel -format Test-Game-2.html >> output
	../../babel -ifid Test-Game-2.html >> output
	../../babel -format Test-Game-UUID.html >> output
	../../babel -ifid Test-Game-UUID.html >> output
	../../babel -format Test-Game-Legacy.html >> output
	../../babel -ifid Test-Game-Legacy.html >> output
	$(VERIFY)

clean:
	-rm -f output
