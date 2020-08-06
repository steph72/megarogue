set -e

if [ ! -f "disc/picohack.d81" ]; then
  mkdir -p disc
  c1541 -format picohack,sk d81 disc/picohack.d81
fi

c1541 <<EOF
attach disc/picohack.d81
delete picoh.prg
write bin/picoh.prg
EOF
