# compile
mkdir build && cd build && CXX=g++-4.8 cmake -DPostgreSQL_TYPE_INCLUDE_DIR=/usr/include/postgresql ..
make

# prepare the test db
psql -U postgres -c "create database tempus_test_db;"
psql -U postgres -c "create extension postgis;" tempus_test_db
cd ../data/tempus_test_db
unzip tempus_test_db.sql.zip
psql -U postgres tempus_test_db < tempus_test_db.sql
for p in patch*.sql; do
    psql -U postgres tempus_test_db < $p
done
cd -

# launch unit tests
ctest -VV
