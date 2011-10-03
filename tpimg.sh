# tpimg.sh
#!/bin/sh
if [ ! -f "./cache/$1" ]; then
    wget "http://10.4.1.100/img/m/$1" -P cache/
fi

./tpsend 'IMG' #MAGIC

