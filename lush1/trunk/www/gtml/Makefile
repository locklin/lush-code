
gtml=./gtml
outdir=../html


HTML= \
    ${outdir}/index.html \
    ${outdir}/credits.html \
    ${outdir}/news.html \
    ${outdir}/links.html \
    ${outdir}/doc.html \
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
${outdir}/news.html: news.gtml latest-news older-news
${outdir}/credits.html: credits.gtml
${outdir}/links.html: links.gtml
${outdir}/doc.html: doc.gtml
${outdir}/screenshots.html: screenshots.gtml


${HTML}: ${INC}

FORCE:
