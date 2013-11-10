sudo apt-get install nginx postgresql postgresql-server-dev-9.1 libpq-dev libgeos-dev g++ libboost-all-dev libglib2.0-dev libfcgi-dev libshp-dev libxml2-dev cmake cmake-curses-gui pyqt4-dev-tools libtool libproj-dev libgdal-dev

TEMPUS_DATA_DIRECTORY=/usr/local/share/tempus/data
TEMPUS=tempus-0.9.0
die()
{
    echo $1 1>&2
    exit 1
}

echo "downloading, compiling and installing tempus"
    #wget http://tempus.ifsttar.fr/tempus-0.9.0.tar.gz && # TODO: uncomment and give proper adress
    tar -xvzf $TEMPUS.tgz &&
    mkdir $TEMPUS/build &&
    cd $TEMPUS/build &&
    cmake .. && make  && sudo make install || die "cannot install tempus"
    cd ../..
    sudo ldconfig 
echo "tempus has been successfully installed"

echo "downloading, compiling and installing postgis"
   wget http://download.osgeo.org/postgis/source/postgis-2.1.0.tar.gz &&
   tar -xvzf postgis-2.1.0.tar.gz &&
   cd postgis-2.1.0 &&
   ./configure && make && sudo make install || die "cannot install postgis"
   cd ..
echo "postgis has been successfully installed"

echo "configuring test database"
    # create the database and populate it
    sudo su - postgres -c "
        dropdb tempus_test_db;
        createdb tempus_test_db;
        psql tempus_test_db -c 'CREATE EXTENSION postgis';
        psql tempus_test_db < $TEMPUS_DATA_DIRECTORY/tempus_test_db.sql
    "
    # wps is run as deamon, so we need to create a role for user 'daemon' in the test database
    sudo su - postgres -c "
        psql tempus_test_db -c 'CREATE USER daemon'
        psql tempus_test_db -c 'GRANT USAGE ON SCHEMA public,tempus TO daemon'
        psql tempus_test_db -c 'GRANT SELECT ON ALL TABLES IN SCHEMA public,tempus TO daemon'
    " || die "cannot configure test database"
echo "test database configured"

echo "installing tempus wps service"
    # install wps service and start it
    sudo cp $TEMPUS/script/wps /etc/init.d/wps &&
    sudo update-rc.d wps defaults 80 
    sudo /etc/init.d/wps restart || die "cannot start wps service"
echo "tempus wps service has been successfully installed"

echo "configuring nginx web server for wps service on 127.0.0.1/wps"
    # patch nginx config
    diff /etc/nginx/sites-available/default \
        $TEMPUS_DATA_DIRECTORY/nginx/sites-available/default \
        | sudo patch /etc/nginx/sites-available/default
    sudo /etc/init.d/nginx restart || die "cannot start ngnix web server"
echo "nginx web server has been configured"

echo "installing qgis plugins for current user"
    mkdir -p ~/.qgis2/python/plugins 
    cp -r $(find tempus-0.9.0/src -name metadata.txt -printf "%h ") ~/.qgis2/python/plugins || die "cannot install plugins for current user" 
echo "qgis plugins have been installed for current user"

exit 0
