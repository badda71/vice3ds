#!/bin/sh
cwd=`pwd`
for i in `find source -type f -size -50c`
do
	ii=`cat $i`
	dir=`dirname "$i"`
	fn=`basename "$i"`
	cd $dir
	rm -rf $fn
	ln -s $ii $fn
	cd $cwd
done
