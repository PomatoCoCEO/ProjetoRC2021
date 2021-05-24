#!/usr/bin/bash
dir_ext="setup/project-files/docker";

for dir in $(ls $dir_ext); do
    cp -r code $dir_ext/$dir/home
done
echo "Done!"