# This is the RPM spec file for tl3-1.6

Name:		tl3
Version:	1.7
Release:	1
Summary: 	A compact Lisp interpreter with graphics capabilities.
Copyright: 	GPL
Distribution: 	TL3
Url: 		http://www.research.att.com/~leonb/TL3
Group:		Development/Languages
Packager:	Leon Bottou <leonb@research.att.com>
Source:		ftp://leon.bottou.com/TL3/tl3-1.7.tar.gz

%description
TL3 is a compact Lisp interpreter with an easy interface to C/C++ and
graphics capabilities.  It is designed for easily prototyping all
kinds of applications with a slight bias towards numerical analysis
and image processing.

%prep
%setup -n tl3

%files 
/usr/lib/tl3
/usr/bin/tl3
%doc README.tl3
%doc README.wintl3
%doc PORTING
%docdir /usr/tl3/help

%build
configure --prefix=/usr
make

%install
make install
