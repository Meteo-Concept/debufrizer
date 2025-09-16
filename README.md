# debufrizer

Parse Météo France reflectivity radar BUFR files and output the corresponding TIFF files


## Dependencies

* A C++23 compiler and standard library
* GDAL library 3.10 (e.g. for Debian: libdal-dev)
  v3.10 is the one that has been tested, previous versions likely to work as
  well to some extent, we don't use very advanced functions from GDAL

## Compilation

```sh
mkdir build
cd build
cmake ..
make
```

## Usage

The debufrizer program expects an input file on the command line, and can
optionally accept a prefix for the output file. The output file will be named
`<prefix><date>.tif` where `<date>` is the date and time found in section 1 of
the BUFR file formatted as `%Y-%m-%d_%H-%M-%S`. The program outputs this date on
the standard output after parsing is done.

```sh
cd examples
../build/debufrizer reflectivity.bufr output_
```

There is an additional script `process.sh` in the examples folder to show how to
use the Météo France API, the debufrizer program and the GDAL utilities to
output a reflectivity and an intensity map (the latter using [Marshall-Palmer
formula](https://en.wikipedia.org/wiki/Raindrop_size_distribution).

To start using the Météo France API, go to their [API portal](https://portail-api.meteofrance.fr/),
additional documentation on their [wiki](https://confluence-meteofrance.atlassian.net/wiki).


## Limitations

This program is probably only useful for Météo France radar BUFR files, and
possibly other OPERA members files as well, although only Météo France
reflectivity files have been tested. It's also unclear how much resistant to
future format changes this program is resistant to. Basically, it just looks
through the file to find the grid size, checks that the projection is the only
one it supports, finds a block of data the same size of the grid and
corresponding to reflectivity data, and builds a TIFF file with that. Caveat
emptor.


# License

The code in the repository is licensed by:
SAS JD Environnement
44C rue de Bray
35510 CESSON-SEVIGNE
FRANCE
contact@meteo-concept.fr

under the GNU Public License (GPL) version 3 or higher. The text of the license
is available in the License file.

© SAS JD Environnement 2025
