@echo off
:call qe -bios bios64.bin -hdd HDD_BOOT.IMG
call qe -bios bios64.bin -fda fdd.img,format=raw -device floppy -device VGA -nodefaults