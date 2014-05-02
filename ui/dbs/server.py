#!/usr/bin/env python

import os, sys, time, glob
from flask import Flask, render_template, request
from werkzeug.utils import secure_filename
from werkzeug.exceptions import BadRequest
from stat import S_ISREG, ST_CTIME, ST_MODE
import subprocess
import uuid

STATIC_PATH = "/static"
STATIC_FOLDER = "../static"
TEMPLATE_FOLDER = "../templates"
UPLOAD_FOLDER = '../static/uploads'
ALLOWED_EXTENSIONS = set(['png', 'jpg', 'jpeg', 'gif'])
PAGE_SIZE = 5
MAX_IMAGE_WIDTH=800

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

def get_image_urls(page):
    files = filter(os.path.isfile, glob.glob(os.path.join(UPLOAD_FOLDER, "*")))
    file_list = []
    for f in files:
        d = os.path.getmtime(f)
        file_list.append((os.path.basename(f),d))
    file_list.sort(key=lambda x: x[1], reverse=True)

    urls = [ "/static/uploads/" + f[0] for f in file_list[page * PAGE_SIZE : page * PAGE_SIZE + PAGE_SIZE]]
    return urls, page < (len(file_list) // PAGE_SIZE) 

def scale_image(filename, width):
    tf = os.path.join(UPLOAD_FOLDER, str(uuid.uuid4()))
    if not subprocess.check_call(["convert", "-size", "%d" % width, filename, "-resize", 
                          "%d" % width, tf]):
        os.unlink(filename);
        os.rename(tf, filename)
    else:
        print "resize failed"

@app.route("/images")
def images():
    urls, have_more = get_image_urls(0)
    if have_more:
        next_page = 1
    else:
        next_page = 0
    return render_template("images", urls=urls, next_page=index)

@app.route("/images/<int:page>")
def image(page):
    urls, have_more = get_image_urls(page)
    if have_more:
        next_page = page + 1
    else:
        next_page = 0

    return render_template("image", urls=urls, next_page=next_page)

@app.route("/chill")
def chill():
    return render_template("index")

@app.route("/ws/upload", methods=['POST'])
def ws_upload():
    file = request.files['file']
    if file and allowed_file(file.filename):
        filename = os.path.join(UPLOAD_FOLDER, secure_filename(file.filename))
        file.save(filename)
        scale_image(filename, MAX_IMAGE_WIDTH)
        return ""
    raise BadRequest("Unsupported file type")

if __name__ == "__main__":
    app.run(debug=True, host="0.0.0.0", port=8080)
