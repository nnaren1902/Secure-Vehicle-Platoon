#!/usr/bin/env python
"""
@file    tracirunner.py
@author  Michael Behrisch
@author  Daniel Krajzewicz
@author  Friedemann Wesner
@version $Id$

Wrapper script for running TraCI tests with TextTest.

SUMO, Simulation of Urban MObility; see http://sumo.sourceforge.net/
Copyright (C) 2008-2012 DLR (http://www.dlr.de/) and contributors
All rights reserved
"""
import os,subprocess,sys,time

args = sys.argv[1:]
if 'endsumo' in args:
    args.remove('endsumo') # legacy
try:
    num_server_args = args.index('TraCITestClient.exe')
except:
    sys.exit("argument 'TraCITestClient.exe' missing")

server_args = args[:num_server_args]
client_args = args[num_server_args:]
binaryDir, server = os.path.split(server_args[0])

client = "TraCITestClient"
if "64" in server:
    client += "64"
if server[-1] == "D":
    client += "D"
if os.name != 'posix':
    client += ".exe"

client_args[0] = os.path.join(binaryDir, client)

#start sumo as server    
serverprocess = subprocess.Popen(server_args, stdout=sys.stdout, stderr=sys.stderr)
for retry in range(10):
    clientProcess = subprocess.Popen(client_args, stdout=sys.stdout, stderr=sys.stderr)
    if serverprocess.poll() != None and clientProcess.poll() == None:
        time.sleep(10)
        if serverprocess.poll() != None and clientProcess.poll() == None:
            print >> sys.stderr, "Client hangs but server terminated for unknown reason"
            clientProcess.kill()
            break
    if clientProcess.wait() == 0:
        break
    time.sleep(1)

#wait for the server to finish
serverprocess.wait()
