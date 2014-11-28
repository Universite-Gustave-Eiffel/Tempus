#!/bin/sh
psql tempus_unit_test -c "CREATE EXTENSION IF NOT EXISTS postgis"
psql tempus_unit_test -c "DROP SCHEMA tempus CASCADE"
psql tempus_unit_test < toy_graph.sql
