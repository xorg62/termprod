# tpquery.sh
#!/bin/sh

# Necessite de ne pas avoir à taper le mot de passe.
./tpsend "`ssh prologue@10.4.2.70 /database/bin/a_prix $1`" 

