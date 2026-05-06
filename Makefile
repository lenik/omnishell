# OmniShell — root Makefile (localization and small dev tasks)
#
# Localization (gettext), catalog: omnishell
#   make l10n-pot     Regenerate po/omnishell.pot from po/POTFILES.in
#   make l10n-merge   msgmerge pot into each po/*.po (updates translations catalog)
#   make l10n-mo      Compile .mo files (see MO_ROOT)
#   make l10n-check   msgfmt -c on all .po (no .mo output)
#   make l10n         pot + merge + mo (typical refresh before commit)
#   make l10n-regen   Refill zh_*/ko_*/ja_* msgstr from pot via po/regen_po.py
#
# Meson also builds/installs .mo when msgfmt is found; from build dir:
#   meson compile -C build l10n

PROJECT_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
PO_DIR := $(PROJECT_ROOT)/po
DOMAIN := omnishell
POT := $(PO_DIR)/$(DOMAIN).pot
POTFILES := $(PO_DIR)/POTFILES.in
LINGUAS := $(PO_DIR)/LINGUAS

LANGS := $(shell sed -e '/^#/d' -e '/^[[:space:]]*$$/d' $(LINGUAS))

XGETTEXT ?= xgettext
MSGFMT ?= msgfmt
MSGMERGE ?= msgmerge

# Compiled catalogs: <MO_ROOT>/<lang>/LC_MESSAGES/<DOMAIN>.mo
# Match Meson layout with: make l10n-mo MO_ROOT=$(PROJECT_ROOT)/build/po
MO_ROOT ?= $(PO_DIR)/mo

.PHONY: l10n l10n-pot l10n-merge l10n-mo l10n-check l10n-regen

l10n: l10n-pot l10n-merge l10n-mo

l10n-pot:
	@test -f $(POTFILES) || { echo "missing $(POTFILES)"; exit 1; }
	$(XGETTEXT) --keyword=_ --from-code=UTF-8 --language=C++ \
		--package-name=$(DOMAIN) \
		-f $(POTFILES) -o $(POT) \
		--directory=$(PROJECT_ROOT)

l10n-merge: l10n-pot
	@for lang in $(LANGS); do \
		test -f $(PO_DIR)/$$lang.po || { echo "missing $(PO_DIR)/$$lang.po"; exit 1; }; \
		$(MSGMERGE) -U --backup=none --add-location=file $(PO_DIR)/$$lang.po $(POT) || exit 1; \
	done

l10n-mo:
	@for lang in $(LANGS); do \
		dest=$(MO_ROOT)/$$lang/LC_MESSAGES; \
		mkdir -p $$dest; \
		$(MSGFMT) -c -o $$dest/$(DOMAIN).mo $(PO_DIR)/$$lang.po || exit 1; \
	done
	@echo "Wrote $(DOMAIN).mo under $(MO_ROOT)/<lang>/LC_MESSAGES/"

l10n-check:
	@for lang in $(LANGS); do \
		$(MSGFMT) -c -o /dev/null $(PO_DIR)/$$lang.po || exit 1; \
	done

l10n-regen:
	@python3 $(PO_DIR)/regen_po.py
