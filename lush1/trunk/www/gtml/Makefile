
gtml=./gtml
outdir=../html


HTML= \
    ${outdir}/index.html \
    ${outdir}/credits.html \
    ${outdir}/links.html \
    ${outdir}/screenshots.html \

INC= \
    include/footer.inc \
    include/gtmlmacros \
    include/header.inc \
    include/menu.inc \
    include/style.inc \
    include/tabs.inc \
    gtml.conf

all: ${HTML}

clean: FORCE
	-rm 2>/dev/null ${HTML}

${HTML}:
	$(gtml) $<

${outdir}/index.html: index.gtml
${outdir}/credits.html: credits.gtml
${outdir}/links.html: links.gtml
${outdir}/screenshots.html: screenshots.gtml


${HTML}: ${INC}

FORCE:
