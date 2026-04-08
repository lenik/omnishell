# Run from repo root: make -C src organize-includes
# Or from : make organize-includes
#
# Localization (gettext): from repo root use  make l10n  (see ../Makefile)

SOURCES := $(shell find . -type f \( -name '*.cpp' -o -name '*.c' -o -name '*.hpp' -o -name '*.h' \) | sort)

.PHONY: format
format:
	organize-includes -n -I $(CURDIR) $(SOURCES)

bulk.h:
	grep -r -h '^#include <' | sort -u | grep -v bas/ >$@

