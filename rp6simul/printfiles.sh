#!/bin/sh
for F in `find $*`
do
    [ "`basename $F`" = ".svn" ] && continue
    [ ! -d "`dirname $F`/.svn" ] && continue
    svn stat $F | grep '?' >/dev/null && continue
    echo $F | grep -v svn | grep -v moc >/dev/null && [ ! -d $F ] && echo \'$F\',
done | sort

