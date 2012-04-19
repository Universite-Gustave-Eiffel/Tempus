# Configuration file for Tempus loader
# Replace following values with specific paths for your system

# import schema
IMPORTSCHEMA = "_tempus_import"

# Path where binaries are located (used below)
BINPATH=""

# PostgreSQL's console client binary location
PSQL=BINPATH + ""

# PostGIS shp2pgsql tool
SHP2PGSQL=BINPATH + ""

# DO NOT MODIFY UNDER THIS LINE
# Autoconfiguration attempts
import os

def is_exe(fpath):
    return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

def which(program):
    fpath, fname = os.path.split(program)
    if fpath:
        if is_exe(program):
            return program
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file

    return None

if not is_exe(PSQL):
    PSQL = which("psql")

if not is_exe(SHP2PGSQL):
    SHP2PGSQL = which("shp2pgsql")

if PSQL is None or SHP2PGSQL is None:
    raise OSError("Could not find psql and shp2pgsql.")
