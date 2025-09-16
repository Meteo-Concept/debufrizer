#!/bin/bash

curl -X 'GET' \
  'https://public-api.meteofrance.fr/public/DPRadar/v1/mosaiques/METROPOLE/observations/REFLECTIVITE/produit?maille=1000' \
  -H 'accept: application/octet-stream+gzip' \
  -H "$METEOFRANCE_AUTH" \
  --clobber \
  -o "reflectivity.bufr.gz"

gzip -d -f "reflectivity.bufr.gz"

date=$(../cmake-build-debug/debufrizer "reflectivity.bufr" "reflectivity_" | sed -E -e 's/.*([0-9]{4}-[0-9]{2}-[0-9]{2}_[0-9]{2}-[0-9]{2}-[0-9]{2}).*/\1/')
input="reflectivity_$date.tif"
projected="reflectivity_${date}_3857.tif"
intensity="intensity_${date}_3857.tif"

gdalwarp -ot Int8 -of COG -srcnodata nan -dstnodata 127 -t_srs EPSG:3857 -r near -overwrite "$input" "$projected"

gdal_calc -A "$projected"  --calc="(10.**(A/10.)/200.)**(5./8)" --type=Float32 --overwrite --outfile="$intensity"

