fatload mmc 0 0x00200000 zImage
fatload mmc 0 0x01000000 devicetree.dtb
fatload mmc 0 0x02000000 uramdisk.image.gz
setenv bootargs 'console=ttyPS0,115200 earlyprintk root=/dev/ram rw'
bootz 0x00200000 0x02000000 0x01000000
