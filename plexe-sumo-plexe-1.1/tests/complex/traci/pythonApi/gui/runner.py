#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os, subprocess, sys, struct, time
sys.path.append(os.path.join(os.path.dirname(sys.argv[0]), "..", "..", "..", "..", "..", "tools"))
import traci, sumolib

sumoBinary = sumolib.checkBinary('sumo-gui')

sumoProcess = subprocess.Popen("%s -S -Q -c sumo.sumocfg" % (sumoBinary), shell=True, stdout=sys.stdout)
traci.init(8813)
for step in range(3):
    print "step", step
    traci.simulationStep()
time.sleep(1) # give the gui a chance to draw itself
print "views", traci.gui.getIDList()
viewID = traci.gui.DEFAULT_VIEW
print "examining", viewID
print "zoom", traci.gui.getZoom(viewID)
print "offset", traci.gui.getOffset(viewID)
print "schema", traci.gui.getSchema(viewID)
print "visible boundary", traci.gui.getBoundary(viewID)

traci.gui.subscribe(viewID)
print traci.gui.getSubscriptionResults(viewID)
for step in range(3,6):
    print "step", step
    traci.simulationStep()
    print traci.gui.getSubscriptionResults(viewID)
traci.close()
