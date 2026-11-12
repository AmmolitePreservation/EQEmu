# pk <pid> <address> <size>: the hex at an address
pk(){ bash /c/Onyx/tools/gameTest/lcEmu.sh command $1 peek $2 $3 | grep -o '"hex": "[0-9a-f]*"' | cut -d'"' -f4; }
# le32 <hex> <byteOffset>: the little-endian dword at an offset of a hex string, as 0x...
le32(){ local s=${1:$(( $2 * 2 )):8}; echo "0x${s:6:2}${s:4:2}${s:2:2}${s:0:2}"; }
