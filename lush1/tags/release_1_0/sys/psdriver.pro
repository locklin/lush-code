%% Creator: Lush
%% Pages: (atend)
%% EndComments

/AnsiCodes [ /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef
 /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef
 /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef
 /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef
 /.notdef /space /exclam /quotedbl /numbersign /dollar /percent /ampersand
 /quoteright /parenleft /parenright /asterisk /plus /comma /minus /period
 /slash /zero /one /two /three /four /five /six /seven /eight /nine /colon
 /semicolon /less /equal /greater /question /at /A /B /C /D /E /F /G /H /I
 /J /K /L /M /N /O /P /Q /R /S /T /U /V /W /X /Y /Z /bracketleft /backslash
 /bracketright /asciicircum /underscore /quoteleft /a /b /c /d /e /f /g /h
 /i /j /k /l /m /n /o /p /q /r /s /t /u /v /w /x /y /z /braceleft /bar
 /braceright /asciitilde /.notdef /.notdef /.notdef /.notdef /.notdef
 /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef
 /.notdef /.notdef /.notdef /.notdef /dotlessi /grave /acute /circumflex
 /tilde /macron /breve /dotaccent /dieresis /.notdef /ring /cedilla /.notdef
 /hungarumlaut /ogonek /caron /space /exclamdown /cent /sterling /currency
 /yen /brokenbar /section /dieresis /copyright /ordfeminine /guillemotleft
 /logicalnot /hyphen /registered /macron /degree /plusminus /twosuperior
 /threesuperior /acute /mu /paragraph /periodcentered /cedilla /onesuperior
 /ordmasculine /guillemotright /onequarter /onehalf /threequarters
 /questiondown /Agrave /Aacute /Acircumflex /Atilde /Adieresis /Aring /AE
 /Ccedilla /Egrave /Eacute /Ecircumflex /Edieresis /Igrave /Iacute
 /Icircumflex /Idieresis /Eth /Ntilde /Ograve /Oacute /Ocircumflex /Otilde
 /Odieresis /multiply /Oslash /Ugrave /Uacute /Ucircumflex /Udieresis
 /Yacute /Thorn /germandbls /agrave /aacute /acircumflex /atilde /adieresis
 /aring /ae /ccedilla /egrave /eacute /ecircumflex /edieresis /igrave /iacute
 /icircumflex /idieresis /eth /ntilde /ograve /oacute /ocircumflex /otilde
 /odieresis /divide /oslash /ugrave /uacute /ucircumflex /udieresis /yacute
 /thorn /ydieresis
] def

/recodedict 5 dict def
/recodefont {
  dup /Encoding get StandardEncoding eq {
    recodedict begin  
    /basedict exch def
    /newdict basedict maxlength dict def
    basedict
    { exch dup dup /Encoding ne exch /FID ne and
      { exch newdict 3 1 roll put } { pop pop } ifelse } forall
    newdict /Encoding AnsiCodes put
    /ansifont newdict definefont
    end
  } if
} bind def

/BEGINPAGE {
  gsave translate scale } bind def
/ENDPAGE {
  grestore showpage } bind def
/SF {
  dup /default eq 
  { pop pop /Courier findfont recodefont 11 scalefont setfont }
  { findfont recodefont exch scalefont setfont } ifelse } bind def
/SC {
  setrgbcolor } bind def
/SCFG { 
  0 setgray } bind def
/SCBG {
  1 setgray } bind def
/SCGRAY { 
  0.5 setgray } bind def
/DL {
  newpath moveto lineto stroke } bind def
/DR {
  newpath moveto exch dup 0 rlineto exch
  0 exch rlineto neg 0 rlineto
  closepath stroke } bind def
/FR {
  newpath moveto exch dup 0 rlineto exch
  0 exch rlineto neg 0 rlineto
  closepath fill } bind def
/DC {
  newpath 0 360 arc stroke } bind def
/FC {
  newpath 0 360 arc fill } bind def
/DA {
  newpath neg exch neg arc stroke } bind def
/FA {
  newpath 4 index 4 index moveto  neg exch neg arc closepath fill } bind def
/backstr 2 string def backstr 1 8 put
/DT {
  moveto gsave 1 -1 scale show grestore } bind def
/CLIP {
  initclip newpath moveto exch dup 0 rlineto exch
  0 exch rlineto neg 0 rlineto
  closepath clip newpath } bind def
/PSTART {
  newpath moveto } bind def
/FP {
  lineto } bind def
/PEND {
  lineto closepath fill } bind def
/PM {
  gsave	translate scale
  8 [ 1 0 0 1 0 0 ] 
  2 index 2 mul string /picstr exch def
  { currentfile picstr readhexstring pop } image
  grestore } bind def
/CPM {
  gsave translate scale
  [ 1 0 0 1 0 0 ]
  3 index 3 mul string /picstr exch def
  { currentfile picstr readhexstring pop }
  false 3 colorimage
  grestore } bind def
/HM {
  gsave	translate gsave 2 copy
    4 index mul exch 5 index mul exch scale
   newpath 0 0 moveto 1 0 rlineto 0 1 rlineto -1 0 rlineto
   closepath 0.5 setgray fill grestore
   dup scale 3 1 roll 0.5 0.5 translate 
   { gsave 2 copy
    { gsave dup
       currentfile picstr readhexstring pop 0 get 
       dup 127 gt { 0 setgray 256 sub neg } { 1 setgray } ifelse 
       exch div dup scale -0.5 -0.5 moveto 
       1 0 rlineto 0 1 rlineto -1 0 rlineto
       closepath fill grestore 
      1 0 translate } repeat 
     pop grestore
    0 1 translate } repeat
  grestore pop
} bind def

11 /default SF 
SCFG 

%%EndProlog
