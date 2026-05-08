SHELL = /bin/bash

.PHONY: FILES.lst FILES.txt FILES

update:
	@while read p; do \
		if [ ! -f "$$p" ]; then \
			echo "install $$p"; \
			install -CD "/usr/share/$$p" "$$p"; \
		fi; \
	done < <( jq -r '.[][].asset?' themes/popular.json ); \
	true

FILES: FILES.txt FILES.lst
	@cp FILES.txt $@

FILES.lst:
	@d=$$PWD; (\
	cd /usr/share; \
	for sub in core ultimate plump; do \
		find streamline-vectors/$$sub/ -type f; \
	done; \
	find heroicons/svg/normal/ -type f \
	) | sort >$$d/$@

FILES.txt:
	@d=$$PWD; out=$$d/$@; \
	echo -n >$$out; \
	for sub in core ultimate plump; do ( \
		cd /usr/share/streamline-vectors/$$sub; \
		echo streamline-vectors/$$sub/:  >>$$out; \
		find -type f | sed -e 's/^\.\//\t/' | sort >>$$out; \
		echo                            >>$$out \
		); done; \
	cd /usr/share/heroicons/svg/normal; \
		echo heroicons/svg/normal/:     >>$$out; \
		find -type f | sed -e 's/^\.\//\t/' | sort >>$$out

