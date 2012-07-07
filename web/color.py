#!/usr/bin/env python
    
from flask import Flask
import os

app = Flask(__name__)

@app.route("/set/<color>")
def set_color(color):
    red = int(color[0:1], 16)
    green = int(color[2:3], 16)
    blue = int(color[4:5], 16)
    os.system("../ledstick-set %d %d %d" % (red, green, blue))
    return ""

@app.route("/")
def index():
    return '''<html><body>Gimme a color: #<input type="text" size="6" id="color">
              <button type="button" onclick="setcolor()">set!</button>
              <script type="text/javascript">
              function setcolor() { 
                  color = $("#color").val();
                  $.ajax({ url: "/set/" + color })
              }</script>
              <script src="//ajax.googleapis.com/ajax/libs/jquery/1.7/jquery.min.js"></script>
              </body></html>''';


if __name__ == "__main__":
    app.run(debug=True, host="0.0.0.0", port=8000)
