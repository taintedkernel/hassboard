#!/bin/bash

RUN_TS=$(stat -c %Y smartgirder)
GIRDER_CMD="smartgirder 1"
./$GIRDER_CMD &
while true
do
    #if [ "$(pgrep smartgirder)" == "" ]; then
    #fi
    sleep 10
    NOW_TS=$(stat -c %Y smartgirder)
    if [ "$NOW_TS" -gt "$RUN_TS" ]; then
        echo "Updated binary detected, restarting"
        kill %1
        RUN_TS=$(stat -c %Y smartgirder)
        ./$GIRDER_CMD &
    fi
done
