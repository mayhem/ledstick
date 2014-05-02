#!/usr/bin/env python

import os
from flask import Flask, render_template, request
from werkzeug.utils import secure_filename
from werkzeug.exceptions import BadRequest

STATIC_PATH = "/static"
STATIC_FOLDER = "../static"
TEMPLATE_FOLDER = "../templates"
UPLOAD_FOLDER = '../images'
ALLOWED_EXTENSIONS = set(['png', 'jpg', 'jpeg', 'gif'])

app = Flask(__name__,
            static_url_path = STATIC_PATH,
            static_folder = STATIC_FOLDER,
            template_folder = TEMPLATE_FOLDER)

def allowed_file(filename):
    return '.' in filename and \
           filename.rsplit('.', 1)[1] in ALLOWED_EXTENSIONS

@app.route("/")
def index():
    return render_template("index")

@app.route("/upload")
def upload():
    return render_template("upload")

@app.route("/images")
def images():
    return render_template("index")

@app.route("/chill")
def chill():
    return render_template("index")

@app.route("/ws/upload", methods=['POST'])
def ws_upload():
    file = request.files['file']
    if file and allowed_file(file.filename):
        filename = secure_filename(file.filename)
        file.save(os.path.join(UPLOAD_FOLDER, filename))
        return ""
    raise BadRequest("Unsupported file type")

if __name__ == "__main__":
    app.run(debug=True, host="0.0.0.0", port=8080)
