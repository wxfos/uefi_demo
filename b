#alias cc=/usr/bin/clang 
objdir=build/obj
iso=build/boot.iso
isodir=build/iso
img=$isodir/fdd.img
fdddir=build/fdd
efidir=build/fdd/efi/boot

cflag='-c -target x86_64-pc-win32-coff  -fno-stack-protector -mno-red-zone -nostdinc -Isrc/efi/inc -Isrc/efi/inc/x86_64 -nostdlib'

cc $cflag -o $objdir/main.o src/main.cpp
#cc $cflag -o $objdir/demo.o src/demo.cpp
#cc $cflag -o $objdir/mlibc.o src/mlibc.cpp

lld-link -subsystem:efi_application -nodefaultlib -dll -entry:efi_main $objdir/main.o $objdir/demo.o $objdir/mlibc.o -out:$efidir/bootx64.efi


if [ ! -f $img ]; then
	dd if=/dev/zero of=$img bs=1k count=1440
	#
	#/usr/local/Cellar/dosfstools/4.2/sbin/mkfs.fat $img
	#
	#-C -B /path/to/bootloader.img 
	mformat -t 1440 -h 1 -s 18 -i $img ::
fi
#mcopy -v -o -i $img -s $fdddir/* ::

#xorriso -as mkisofs -iso-level 3 -V "boot" -R -J -joliet-long -e BOOT/BOOTX64.EFI -b fdd.img  -o $iso $isodir
#xorriso -as mkisofs  -b fdd.img  -o $iso $isodir

