#!/bin/bash
cd ~/chanserv/listchecker
# Выдача количества используемой памяти процессом listchecker
export LIBGL_ALWAYS_SOFTWARE=1
ps -eo size,pid,user,command --sort -size |     awk '{ hr=$1/1024 ; printf("%13.2f Mb ",hr) } { for ( x=4 ; x<=NF ; x++ ) { printf("%s ",$x) } print "" }' |    cut -d "" -f2 | cut -d "-" -f1 | grep -w "./listchecker"
ps -eo size,pid,user,command --sort -size |     awk '{ hr=$1/1024 ; printf("%13.2f Mb ",hr) } { for ( x=4 ; x<=NF ; x++ ) { printf("%s ",$x) } print "" }' |    cut -d "" -f2 | cut -d "-" -f1 | grep -w "bin/listchecker"

