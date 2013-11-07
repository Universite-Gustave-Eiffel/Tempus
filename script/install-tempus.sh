sudo apt-get install nginx postgresql libpq-dev postgis g++ libboost-all-dev libglib2.0-dev libfcgi-dev libshp-dev libxml2-dev cmake cmake-curses-gui pyqt4-dev-tools

# begin database configuration as user posgres
    sudo su - postgres
	dropdb tempus_test_db
	createdb tempus_test_db
	psql tempus_test_db -c "CREATE EXTENSION postgis" # TODO: fix for postgres 9.1
	psql tempus_test_db < /usr/local/share/tempus/data/tempus_test_db.sql

	# wps is run as deamon under user demon, so we need to create a role for this user in the test database
	psql tempus_test_db -c "CREATE USER daemon"
	psql tempus_test_db -c "GRANT USAGE ON SCHEMA public,tempus TO daemon"
        psql tempus_test_db -c "GRANT SELECT ON ALL TABLES IN SCHEMA public,tempus TO daemon"
    logout 
# end database configuration

# begin compile and install tempus
    #wget http://tempus.ifsttar.fr/tempus-0.9.0.tar.gz # TODO: uncomment and give proper adress
    tar -xvzf tempus-0.9.0.tar.gz
    cd tempus-0.9.0
    mkdir build
    cd build
    cmake .. && make && sudo make install
    ldconfig

    # service is ran as user daemon, we need     
    # to define TEMPUS_DATA_DIRECTORY for everyone
    if [ -z $(grep TEMPUS_DATA_DIRECTORY /etc/environment) ]; 
    then 
        echo "echo TEMPUS_DATA_DIRECTORY=/usr/local/share/tempus/data >> /etc/environment " | sudo sh ; 
    else 
        echo "found it"; 
    fi

    # install service
    sudo chmod a+x /etc/init.d/wps
    sudo ln -s /etc/init.d/wps /etc/rc2.d/S20wps
    sudo /etc/init.d/wps start

    # install qgis plugin for current user
    mkdir -p ~/.qgis2/python/plugins
    cp -r $(find ../src -name metadata.txt -printf "%h ") ~/.qgis2/python/plugins
# end compile and install tempus

# begin nginx configuration
    # patch nginx config to create 127.0.0.1/wps
    diff /etc/nginx/sites-available/default \
        ../data/nginx/sites-available/default \
        | sudo patch /etc/nginx/sites-available/default
    sudo /etc/init.d/nginx restart
# end nginx configuration
