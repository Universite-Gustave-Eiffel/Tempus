Getting Started
===============

This document provides a straightforward way to play with Tempus. It is not devised cover all topics.

## Installation dependencies

### On debian wheezy using backports and postgresql-9.4

See the [installation](Installation.md) section for further information.

```bash
sudo sh -c 'echo "deb http://apt.postgresql.org/pub/repos/apt/ wheezy-pgdg main" > /etc/apt/sources.list.d/pgdg.list'
sudo sh -c 'echo "deb http://http.debian.net/debian wheezy-backports main" > /etc/apt/sources.list.d/backports.list'
sudo apt-get install -y wget ca-certificates
wget --quiet -O - https://www.postgresql.org/media/keys/ACCC4CF8.asc | sudo apt-key add -
sudo apt-get update
sudo apt-get install -t wheezy-backports -y \
    libc++-dev g++ libboost-all-dev cmake cmake-curses-gui \
    libxml2-dev libpq-dev libgeos-dev libshp-dev libproj-dev libgdal-dev \
    postgresql-9.4 postgresql-server-dev-9.4 \
    libfcgi-dev nginx \
    libtool pyqt4-dev-tools

# on path to tempus directory
cd /path/to/Tempus
mkdir build && cd build
cmake .. \
        -DCMAKE_INSTALL_PREFIX=$PREFIX \
        -DPostgreSQL_TYPE_INCLUDE_DIR=/usr/include/postgresql/9.4

make
```

## Configure Postgresql

See also the [installation](Installation.md) section for further information.

``` bash
sudo su - postgres -c "createuser -sw lbs"
sudo su - postgres -c "createdb -O lbs tempus"

#Check if you can be loged in as lbs user
psql -U lbs -d tempus -c "create extension postgis"
```


## Load your data using the right loader options

### Tomtom Loader

``` bash
# For more information use --help option
python2.7 src/loader/load_tempus \
    -s /data/tomtom/201409/mn/eur2014_09/shpd/mn/fra/f20
    -t tomtom -m 1409 -d "user=lbs dbname=tempus" -R
```

See [this section](README_loader.md) that describes how the loader is used to build the test database.

## Configure fastcgi

### Using nginx

Create the file /etc/nginx/sites-enabled/default with [this content](../data/nginx/sites-available/default).

``` bash
hme@socompa:~/dev/Tempus$ sudo mkdir -p /etc/nginx/sites-enabled/
hme@socompa:~/dev/Tempus$ sudo cp data/nginx/sites-available/default /etc/nginx/sites-enabled/default
hme@socompa:~/dev/Tempus$ sudo service nginx restart
```


Make sure that `/etc/nginx/sites-enabled/*` files are included.
You must have in your `/etc/nginx/nginx.conf` file, in the `http` section:

```
http{
    # ...
    # ...
    include /etc/nginx/sites-enabled/*;
}
```

### Using apache

See the [installation](Installation.md) section for further information.


## Start Tempus

``` bash
TEMPUS_DATA_DIRECTORY=../data/ ./bin/wps \
    -c ./lib -p 7000  -t 4 -d "dbname=tempus" \
    -l sample_multi_plugin -l sample_pt_plugin -l sample_road_plugin
```

## Use Tempus in QGIS

See [Documentation](Documentation.md)
See [Installation](Installation.md) QGIS plugin installation section