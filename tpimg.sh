# tpimg.sh
#!/bin/sh
if [ ! -f "./$1" ]; then
    wget "http://10.4.1.100/img/m/$1" 
fi

./tpsend 'IMG' #MAGIC

