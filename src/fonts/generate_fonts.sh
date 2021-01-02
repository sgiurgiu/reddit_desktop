#!/bin/bash

for f in `find /home/sergiu/Downloads/Notofonts/ -iname '*.ttf' -printf "%f\n"`
do
    fname="$f.h"
    echo $fname
    /home/sergiu/projects/imgui/misc/fonts/a.out /home/sergiu/Downloads/Notofonts/$f $f > $fname
    sed -i 's/[-\.]/_/g' $fname
done 
