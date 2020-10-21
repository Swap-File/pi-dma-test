# pi-dma
References:

DMA:
https://github.com/raspberrypi/linux/issues/3570

Char Device:
https://github.com/derekmolloy/exploringBB/tree/master/extras/kernel/ebbchar

config.txt changes:
```
dtparam=spi=on
dtoverlay=spi1-2cs  (for dual SPI DMA on pi3 and older)
force_turbo=1
dtoverlay=dma       (the name of our overlay)
#dtparam=spi_dma4   (for pi4 use, first SPI device only)
#total_mem=1024     (for pi4 use, if you do not use dma_alloc_coherent)
```

Compiling an overlay named dma:

```
dtc -I dts -O dtb -o dma.dtbo dma.dts
sudo cp ./dma.dtbo /boot/overlays/dma.dtbo 
```

Compiling and loading the module named dma:

```
sudo rmmod dma.ko
sudo make
sudo modprobe -v regmap-spi
sudo insmod dma.ko
sudo chmod 777  /dev/ebbchar
echo 'test' > /dev/ebbchar
```


