These binaries are used to make the bootable floppy or usb disk. 
The binaries of boot sector are built from Duet\Bootsector\BootSector.inf at r8617 with following steps:
1) enter edk2 workspace directory from command line windows.
2) run "edksetup.bat"
3) run "build -p Duet/Duet.dsc -a IA32 -m Duet/BootSector/BootSector.inf"



