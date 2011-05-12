dist-hook:
	@if test -d "$(srcdir)/.git"; \
	then \
		echo Creating ChangeLog && \
			( cd "$(top_srcdir)" && \
			 echo '# Generated from git history.'; echo; \
			  $(top_srcdir)/missing --run git log --stat ) > ChangeLog.tmp \
			&& mv -f ChangeLog.tmp $(distdir)/ChangeLog \
			|| ( rm -f ChangeLog.tmp ; \
			     echo Failed to generate ChangeLog >&2 ); \
	else \
		echo A git history is required to generate a ChangeLog >&2; \
	fi

SUBDIRS = src
