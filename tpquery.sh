# tpquery.sh
#!/bin/sh

# Necessite de ne pas avoir à taper le mot de passe.
./tpsend "`ssh prologue@217.128.41.73 /database/bin/a_prix $1`"

