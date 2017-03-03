from flask import Flask, request

app = Flask(__name__)

# init python plugin

@app.route('/')
def hello_world():
  return 'Hello, World!'

@app.route('/request')
def make_request():

  print(request.args['start'])
  print(request.args['dest'])
  return 'Itinerary: '

