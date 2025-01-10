magick -size 1000x1000 xc:none -draw "roundrectangle 0,0,1000,1000,500,500" _mask.png 
magick radial-1000.png -alpha Set _mask.png -compose DstIn -composite radial-round.png

magick convert radial-round.png -define icon:auto-resize=32,64,256,512 -compress zip icon.ico
