#!/bin/bash

for f in `find /home/sergiu/Downloads/Roboto_Mono/static/ -iname '*.ttf' -printf "%f\n"`
do
    fname="$f.h"
    echo $fname
    /home/sergiu/projects/imgui/misc/fonts/a.out /home/sergiu/Downloads/Roboto_Mono/static/$f $f > $fname
    sed -i 's/[-\.]/_/g' $fname
done 
