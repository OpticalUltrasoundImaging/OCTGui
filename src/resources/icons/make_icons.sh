magick -size 1000x1000 xc:none -draw "roundrectangle 0,0,1000,1000,500,500" _mask.png 
magick radial-1000.png -alpha Set _mask.png -compose DstIn -composite radial-round.png

### Make mac icon file
DIR=OCTGui.iconset
mkdir $DIR
magick radial-round.png -resize 32x32 $DIR/icon_32x32.png
magick radial-round.png -resize 128x128 $DIR/icon_128x128.png
magick radial-round.png -resize 256x256 $DIR/icon_256x256@.png
magick radial-round.png -resize 512x512 $DIR/icon_512x512.png
iconutil -c icns $DIR
