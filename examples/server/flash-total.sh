#!/bin/bash
LIST=$(make server.flashprof)
LIST=$(echo $LIST | sed 's/ /\\n/g')
echo -e $LIST | tail -n$[ $(echo -e $LIST | wc -l) - 2 ] | awk '{print $1}' | awk '{tot = tot + $1}END{print tot}'
