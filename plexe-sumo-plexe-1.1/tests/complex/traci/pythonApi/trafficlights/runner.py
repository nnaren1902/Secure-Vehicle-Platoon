#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os,subprocess,sys,shutil, struct
sys.path.append(os.path.join(os.path.dirname(sys.argv[0]), "..", "..", "..", "..", "..", "tools"))
import traci, sumolib

sumoBinary = sumolib.checkBinary('sumo')

sumoProcess = subprocess.Popen("%s -c sumo.sumocfg" % (sumoBinary), shell=True, stdout=sys.stdout)
traci.init(8813)
for step in range(3):
    print "step", step
    traci.simulationStep()
print "trafficlights", traci.trafficlights.getIDList()
tlsID = "0"
print "examining", tlsID
print "ryg", traci.trafficlights.getRedYellowGreenState(tlsID)
print "rygdef", traci.trafficlights.getCompleteRedYellowGreenDefinition(tlsID)
print "lanes", traci.trafficlights.getControlledLanes(tlsID)
print "links", traci.trafficlights.getControlledLinks(tlsID)
print "program", traci.trafficlights.getProgram(tlsID)
print "phase", traci.trafficlights.getPhase(tlsID)
print "switch", traci.trafficlights.getNextSwitch(tlsID)
traci.trafficlights.subscribe(tlsID)
print traci.trafficlights.getSubscriptionResults(tlsID)
for step in range(3,6):
    print "step", step
    traci.simulationStep()
    print traci.trafficlights.getSubscriptionResults(tlsID)
traci.close()
