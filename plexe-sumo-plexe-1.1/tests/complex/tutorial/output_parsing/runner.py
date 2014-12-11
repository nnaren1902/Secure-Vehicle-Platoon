#!/usr/bin/env python
"""
@file    runner.py
@author  Michael Behrisch
@date    2012-12-09
@version $Id$

This script is a test runner for the output_parsing tutorial.

SUMO, Simulation of Urban MObility; see http://sumo.sourceforge.net/
Copyright (C) 2008-2012 DLR (http://www.dlr.de/) and contributors
All rights reserved
"""


import os,subprocess,sys,time
import shutil
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..', "tools"))
sys.path.append(os.path.join(os.environ.get("SUMO_HOME", os.path.join(os.path.dirname(__file__), "..", "..", "..")), "tools"))
from sumolib import checkBinary


netconvertBinary = checkBinary('netconvert')
sumoBinary = checkBinary('sumo')
# build/check network
retcode = subprocess.call([netconvertBinary, "-c", "data/circular.netccfg"], stdout=sys.stdout, stderr=sys.stderr)
try: shutil.copy("data/circular.net.xml", "net.net.xml")
except: print "Missing 'circular.net.xml'"
print ">> Netbuilding closed with status %s" % retcode
sys.stdout.flush()
# run simulation
retcode = subprocess.call([sumoBinary, "-c", "data/output_file.sumocfg","--no-step-log"], stdout=sys.stdout, stderr=sys.stderr)
print ">> Simulation closed with status %s" % retcode
sys.stdout.flush()
