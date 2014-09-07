# Created by: Emanuel Haupt <ehaupt@critical.ch>
# $FreeBSD: head/net/httping/Makefile 342470 2014-02-03 20:49:13Z ehaupt $

OLDPORTNAME=	httping
PORTNAME=		multihttping
PORTVERSION=	2.3.4
CATEGORIES=	net
MASTER_SITES=	http://www.vanheusden.com/httping/ \
		CRITICAL
EXTRACT_SUFX=	.tgz

DISTFILES=	${PKGNAMEPREFIX}$(OLDPORTNAME)${PKGNAMESUFFIX}-${PORTVERSION}$(EXTRACT_SUFX)

MAINTAINER=	multihttping@gmail.com
COMMENT=	Ping-like tool for multiple HTTP requests

LICENSE=	GPLv2

LIB_DEPENDS=libjansson.so:${PORTSDIR}/devel/jansson

USES=		gmake gettext

PLIST_FILES=bin/multihttping

OPTIONS_DEFINE=	NCURSES FFTW SSL

OPTIONS_DEFAULT=NCURSES SSL

.include <bsd.port.options.mk>

.if ${PORT_OPTIONS:MNCURSES}
MAKE_ENV+=	NC=yes
CFLAGS+=	-I${LOCALBASE}/include
LDFLAGS+=	-L${LOCALBASE}/lib
.else
MAKE_ENV+=	NC=no
.endif

.if ${PORT_OPTIONS:MFFTW}
MAKE_ENV+=	FW=yes
LIB_DEPENDS+=	fftw3:${PORTSDIR}/math/fftw3
.else
MAKE_ENV+=	FW=no
.endif

.if ${PORT_OPTIONS:MSSL}
MAKE_ENV+=	SSL=yes
.else
MAKE_ENV+=	SSL=no
.endif

post-extract:
	mv "work/$(OLDPORTNAME)-$(PORTVERSION)" "$(WRKSRC)"

post-patch:
	cp files/multihost.c ${WRKSRC}/
	cp files/multihost.h ${WRKSRC}/
	cp files/test_intl.c ${WRKSRC}/

do-install:
	${INSTALL_PROGRAM} ${WRKSRC}/${PORTNAME} ${STAGEDIR}${PREFIX}/bin
	${INSTALL_MAN} ${WRKSRC}/${OLDPORTNAME}.1 ${STAGEDIR}${MANPREFIX}/man/man1


.include <bsd.port.mk>
