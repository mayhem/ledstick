#!/usr/bin/env python

import os
from flask import Flask, render_template, request
from werkzeug.utils import secure_filename
from werkzeug.exceptions import BadRequest

STATIC_PATH = "/static"
STATIC_FOLDER = "../static"
TEMPLATE_FOLDER = "../templates"
UPLOAD_FOLDER = '../static/uploads'
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

def get_image_url(index):
    files = [ f for f in os.listdir(UPLOAD_FOLDER) if os.path.isfile(os.path.join(UPLOAD_FOLDER,f)) ]
    if index >= len(files):
        return "", len(files)
    return "/static/uploads/" + files[index], len(files)

@app.route("/images")
def images():
    url, count = get_image_url(0)
    if count > 1:
        next_index = 1
    else:
        next_index = 0
    print "images next: %d" % next_index
    return render_template("images", image_url=url, next_index=index)

@app.route("/image/<int:index>")
def image(index):
    url, count = get_image_url(index)
    print count
    if index < count - 1:
        next_index = index + 1
    else:
        next_index = 0
    print "image %s next: %d" % (index, next_index)
    return render_template("image", image_url=url, next_index=next_index)

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
