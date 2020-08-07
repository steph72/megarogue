set -e

if [ ! -f "disc/megarogue.d81" ]; then
  mkdir -p disc
  c1541 -format megarogue,sk d81 disc/megarogue.d81
fi

c1541 <<EOF
attach disc/megarogue.d81
delete mrogue.prg
write bin/mrogue.prg
EOF
