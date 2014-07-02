#!/usr/bin/env python

import os, sys, time, glob
from flask import Flask, render_template, request, redirect
from werkzeug.utils import secure_filename
from werkzeug.exceptions import BadRequest
from stat import S_ISREG, ST_CTIME, ST_MODE
import subprocess
import uuid

STATIC_PATH = "/static"
STATIC_FOLDER = "../static"
TEMPLATE_FOLDER = "../templates"
UPLOAD_FOLDER = '../static/uploads'
BITMAP_FOLDER = '../static/bitmaps'
ALLOWED_EXTENSIONS = set(['png', 'jpg', 'jpeg', 'gif'])
PAGE_SIZE = 5
MAX_IMAGE_WIDTH=800

app = Flask(__name__,
            static_url_path = STATIC_PATH,
            static_folder = STATIC_FOLDER,
            template_folder = TEMPLATE_FOLDER)

def allowed_file(filename):
    return '.' in filename and \
           filename.rsplit('.', 1)[1].lower() in ALLOWED_EXTENSIONS

def get_uuids(page, folder):
    files = filter(os.path.isfile, glob.glob(os.path.join(folder, "*")))
    file_list = []
    for f in files:
        d = os.path.getmtime(f)
        file_list.append((os.path.basename(f),d))
    file_list.sort(key=lambda x: x[1], reverse=True)

    uuids = [ os.path.basename(f[0]).split(".")[0] for f in file_list[page * PAGE_SIZE : page * PAGE_SIZE + PAGE_SIZE]]
    return uuids, page < (len(file_list) // PAGE_SIZE) 

def scale_image(filename, width):
    tf = os.path.join(UPLOAD_FOLDER, str(uuid.uuid4()) + ".jpg")
    try:
        if filename.lower().endswith(".jpg") or filename.lower().endswith(".jpeg"):
            subprocess.check_call(["epeg", "-m %d" % width, filename, tf])
        else:
            subprocess.check_call(["convert", filename, "-resize", "%d" % width, tf])
        os.unlink(filename);
    except subprocess.CalledProcessError:
        os.unlink(filename);
        raise BadRequest("Cannot process image. Please try uploading another image. Only .png, .jpg and .gif files are supported.") 

def scale_and_crop_to_png(uuid, x, y, w, h):
    sf = os.path.join(UPLOAD_FOLDER, uuid + ".jpg")
    tf = os.path.join(BITMAP_FOLDER, uuid + ".png")
    try:
        subprocess.check_call(["convert", sf, "-crop", "%dx%d+%d+%d" % (w, h, x, y), "-resize", "x144", tf])
    except subprocess.CalledProcessError, e:
        raise BadRequest("Cannot crop image:" + str(e)) 

def get_image_dims(uuid):
    sf = os.path.join(UPLOAD_FOLDER, uuid + ".jpg")
    try:
        ret = subprocess.check_output(["identify", sf])
        return ret.split(" ")[2].split("x")
    except subprocess.CalledProcessError, e:
        return 0, 0

@app.route("/")
def index():
    uuids, have_more = get_uuids(0, BITMAP_FOLDER)
    if have_more:
        next_page = 1
    else:
        next_page = 0
    return render_template("index", uuids=uuids, next_page=index)

@app.route("/bitmaps/<int:page>")
def image(page):
    uuids, have_more = get_uuids(page, BITMAP_FOLDER)
    if have_more:
        next_page = page + 1
    else:
        next_page = 0

    return render_template("bitmap", uuids=uuids, next_page=next_page)

@app.route("/upload")
def upload():
    return render_template("upload")

@app.route("/other")
def other():
    return render_template("other")

@app.route("/images")
def images():
    uuids, have_more = get_uuids(0, UPLOAD_FOLDER)
    if have_more:
        next_page = 1
    else:
        next_page = 0
    return render_template("images", uuids=uuids, next_page=index)

@app.route("/images/<int:page>")
def image(page):
    uuids, have_more = get_uuids(page, UPLOAD_FOLDER)
    if have_more:
        next_page = page + 1
    else:
        next_page = 0

    return render_template("image", uuids=uuids, next_page=next_page)

@app.route("/crop/<uuid>")
def crop(uuid):
    w, h = get_image_dims(uuid)
    return render_template("crop", uuid=uuid, width=w, height=h)

@app.route("/crop/<uuid>/<int:x>/<int:y>/<int:w>/<int:h>")
def crop_image(uuid, x, y, w, h):
    scale_and_crop_to_png(uuid, x, y, w, h)
    return redirect("/")

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
    if not os.path.exists(UPLOAD_FOLDER):
        os.mkdir(UPLOAD_FOLDER)
    if not os.path.exists(BITMAP_FOLDER):
        os.mkdir(BITMAP_FOLDER)
    app.run(debug=True, host="0.0.0.0", port=8080)
