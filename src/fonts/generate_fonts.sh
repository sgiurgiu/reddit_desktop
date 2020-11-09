#!/bin/bash

for f in `find ~/Downloads/Roboto/ -iname '*.ttf' -printf "%f\n"`
do
    fname="$f.h"
    echo $fname
    /home/sergiu/projects/imgui/misc/fonts/a.out /home/sergiu/Downloads/Roboto/$f $f > $fname  
    #xxd -i /home/sergiu/Downloads/Roboto/$f  $fname 
done 
