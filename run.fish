#!/usr/bin/fish

while true; chrt -f 99 ./smartgirder 1; echo -e "shutdown complete, sleeping\n"; sleep 2; end

