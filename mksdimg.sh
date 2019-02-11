dd if=/dev/zero bs=1k count=65536 of=sd.img
echo ';;b;;' | sfdisk sd.img
mformat -i sd.img@@1M -t 120 -h 16 -s 63
mcopy -i sd.img@@1M -s test_kernel/* ::/

