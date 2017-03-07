# Web Tempus

This is a Flask application that present a web api to allow web app to query tempus core.

## How to install

Clone this repo and [follow instructions](../../docs/BUILDING.md) to build the core of tempus.

Create a virtualenv:

```bash
virtualenv venv
```

/!\ only python2 is supported at the moment.

then install pytempus in your virtualenv:

```bash
pip install ../pytempus
```

Then install other dependencies

```bash
pip install .
```

## How to run

Make sure `LD_LIBRARY_PATH` is correctly set. It should point to `<path-to-your-tempus-build-dir>/lib`.

Make sure you have a correctly populated database (see `docs/` in this repo).


```bash
FLASK_APP=webtempus/app.py flask run
```
