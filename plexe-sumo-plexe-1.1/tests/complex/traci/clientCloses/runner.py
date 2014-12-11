#!/usr/bin/env python

import os, subprocess, sys, time

sumoHome = os.path.abspath(os.path.join(os.path.dirname(sys.argv[0]), '..', '..', '..', '..'))
sys.path.append(os.path.join(sumoHome, "tools"))
import traci

if sys.argv[1]=="sumo":
    sumoBinary = os.environ.get("SUMO_BINARY", os.path.join(sumoHome, 'bin', 'sumo'))
    addOption = ""
else:
    sumoBinary = os.environ.get("GUISIM_BINARY", os.path.join(sumoHome, 'bin', 'sumo-gui'))
    addOption = "-S -Q"
PORT = 8813
DELTA_T = 1000

def patchFile(ifile, ofile, replacements):
    fdi = open(ifile)
    fdo = open(ofile, "w")
    fdo.write(fdi.read() % replacements)
    fdi.close()
    fdo.close()

def runSingle(traciEndTime):
    step = 0
    sumoProcess = subprocess.Popen("%s -c used.sumocfg %s" % (sumoBinary, addOption), shell=True, stdout=sys.stdout)
    traci.init(PORT)
    while not step>traciEndTime:
        traci.simulationStep()
        step += 1
    print "Print ended at step %s" % (traci.simulation.getCurrentTime() / DELTA_T)
    traci.close()
    sys.stdout.flush()


print "=========== long route ==========="
fdo = open("input_routes.rou.xml", "w")
print >> fdo, '<routes>"'
print >> fdo, '   <route id="horizontal" edges="2fi 2si 1o 1fi 1si 3o 3fi 3si 4o 4fi 4si"/>'
print >> fdo, '   <vehicle id="horiz" route="horizontal" depart="0"/>'
print >> fdo, '</routes>'
fdo.close()
print "----------- SUMO end time is smaller than TraCI's -----------"
sys.stdout.flush()
patchFile("sumo.sumocfg", "used.sumocfg", { "end": "<end value='50'/>" } )
runSingle(99)
print "----------- SUMO end time is not given -----------"
sys.stdout.flush()
patchFile("sumo.sumocfg", "used.sumocfg", { "end": "" } )
runSingle(99)


print "=========== empty routes in SUMO ==========="
fdo = open("input_routes.rou.xml", "w")
print >> fdo, '<routes>"'
print >> fdo, '</routes>'
fdo.close()
print "----------- SUMO end time is smaller than TraCI's -----------"
sys.stdout.flush()
patchFile("sumo.sumocfg", "used.sumocfg", { "end": "<end value='50'/>" } )
runSingle(99)
print "----------- SUMO end time is not given -----------"
sys.stdout.flush()
patchFile("sumo.sumocfg", "used.sumocfg", { "end": "" } )
runSingle(99)


print "=========== vehicle leaves before TraCI ends ==========="
fdo = open("input_routes.rou.xml", "w")
print >> fdo, '<routes>"'
print >> fdo, '   <route id="horizontal" edges="2fi 2si"/>'
print >> fdo, '   <vehicle id="horiz" route="horizontal" depart="0"/>'
print >> fdo, '</routes>'
fdo.close()
print "----------- SUMO end time is smaller than TraCI's -----------"
sys.stdout.flush()
patchFile("sumo.sumocfg", "used.sumocfg", { "end": "<end value='50'/>" } )
runSingle(99)
print "----------- SUMO end time is not given -----------"
sys.stdout.flush()
patchFile("sumo.sumocfg", "used.sumocfg", { "end": "" } )
runSingle(99)
