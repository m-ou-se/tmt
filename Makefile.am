dist-hook:
	@echo Creating ChangeLog && \
		( cd "$(top_srcdir)" && ./missing --run git log --date=short --format="%ad  %an  <%ae>%n%x09* %s%n" ) > ChangeLog.tmp \
		&& mv -f ChangeLog.tmp $(distdir)/ChangeLog \
		|| ( rm -f ChangeLog.tmp ; echo Failed to generate ChangeLog >&2 );

SUBDIRS = src
