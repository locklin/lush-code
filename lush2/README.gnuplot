
In Lush 2.0 the gnuplot interface is not supporting 
uni-directional communication and requires a special
gnuplot build for the interface to work smoothly. It
is therefore recommended to build gnuplot 4.2.6 with
GNU libreadline for use with Lush.

If you decide to build gnuplot yourself, don't forget to
install the dev-packages for some gnuplot dependencies 
before building gnuplot. Recommended packages:
* libreadline (required)
* cairo
* pango
* wxWidges
* libgd
