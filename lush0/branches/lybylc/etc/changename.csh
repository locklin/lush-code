#!/bin/tcsh
foreach i ( $1/* )
 sed -f changename.sed $i > $2/$i:t
end
