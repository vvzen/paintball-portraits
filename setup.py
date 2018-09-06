# please run this program using python3!
# CHANGE THIS PATH TO YOUR OPENFRAMEWORKS PATH
OF_ROOT="/Users/vv/code/of/latest_openFrameworks/test"

import os
import re
import sys
import subprocess

addons_path = "{}/addons".format(OF_ROOT)

target_addons = { 
    "ofxIO": "https://github.com/bakercp/ofxIO",
    "ofxSerial" : "https://github.com/bakercp/ofxSerial",
    "ofxCv" : "https://github.com/kylemcdonald/ofxCv",
    "ofxPS3EyeGrabber" : "https://github.com/bakercp/ofxPS3EyeGrabber",
    "ofxFaceTracker" : "https://github.com/kylemcdonald/ofxFaceTracker"
}

num_addons_installed = 0

# see if the user has git installed
try:
    null = open("/dev/null", "w") # pipe output to /dev/null for silence
    subprocess.Popen("git", stdout=null, stderr=null)
    null.close()
except OSError:
    print("you need to have git installed to run this script!\nexiting..")

choice = input("this program is going to install these addons: {}\nare you ok with it? y/n\n".format(", ".join(target_addons.keys())))
if choice == "y":
    user_addons = []
    try:
        user_addons = [f for f in os.listdir(addons_path) if re.match("ofx*", f)]
    except OSError:
        print("I didn't find anything in the path you've given me: {}".format(addons_path))
        print("exiting..")
        sys.exit(0)

    # check if use has already the addon
    for addon in target_addons.keys():
        num_tabs = 2 if len(addon) > 10 else 3
        # check if addons are downloaded
        if addon not in user_addons:
            print("-- {}{}addon not found, downloading it ---> git clone {}".format(addon, num_tabs*"\t", target_addons[addon]))
            # download addon using git
            subprocess.call(["git", "clone", "{}".format(target_addons[addon])], cwd=addons_path)
            num_addons_installed+=1
        else:
            print("-- {}{}already downloaded, skipping".format(addon, num_tabs*"\t"))

    print("finished, installed {} addons".format(num_addons_installed))
else:
    print("user aborted operation")