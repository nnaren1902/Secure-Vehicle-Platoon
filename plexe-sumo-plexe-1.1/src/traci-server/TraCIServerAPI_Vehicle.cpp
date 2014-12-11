/****************************************************************************/
/// @file    TraCIServerAPI_Vehicle.cpp
/// @author  Daniel Krajzewicz
/// @author  Laura Bieker
/// @author  Christoph Sommer
/// @author  Michael Behrisch
/// @author  Bjoern Hendriks
/// @author  Michele Segata
/// @date    07.05.2009
/// @version $Id$
///
// APIs for getting/setting vehicle values via TraCI
/****************************************************************************/
// SUMO, Simulation of Urban MObility; see http://sumo.sourceforge.net/
// Copyright (C) 2001-2013 DLR (http://www.dlr.de/) and contributors
// Copyright (C) 2012-2014 Michele Segata (segata@ccs-labs.org)
/****************************************************************************/
//
//   This file is part of SUMO.
//   SUMO is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
/****************************************************************************/


// ===========================================================================
// included modules
// ===========================================================================
#ifdef _MSC_VER
#include <windows_config.h>
#else
#include <config.h>
#endif

#ifndef NO_TRACI

#include <microsim/MSNet.h>
#include <microsim/MSInsertionControl.h>
#include <microsim/MSVehicle.h>
#include <microsim/MSLane.h>
#include <microsim/MSEdge.h>
#include <microsim/MSEdgeWeightsStorage.h>
#include <microsim/MSAbstractLaneChangeModel.h>
#include <utils/geom/PositionVector.h>
#include <utils/common/DijkstraRouterTT.h>
#include <utils/common/DijkstraRouterEffort.h>
#include <utils/common/HelpersHBEFA.h>
#include <utils/common/HelpersHarmonoise.h>
#include <utils/common/SUMOVehicleParameter.h>
#include "TraCIConstants.h"
#include "TraCIServerAPI_Simulation.h"
#include "TraCIServerAPI_Vehicle.h"
#include "TraCIServerAPI_VehicleType.h"

#include <microsim/cfmodels/CC_Const.h>
#include <microsim/cfmodels/MSCFModel_CC.h>

#ifdef CHECK_MEMORY_LEAKS
#include <foreign/nvwa/debug_new.h>
#endif // CHECK_MEMORY_LEAKS


// ===========================================================================
// used namespaces
// ===========================================================================
using namespace traci;



std::map<std::string, std::vector<MSLane*> > TraCIServerAPI_Vehicle::gVTDMap;

// ===========================================================================
// method definitions
// ===========================================================================
bool
TraCIServerAPI_Vehicle::processGet(TraCIServer& server, tcpip::Storage& inputStorage,
                                   tcpip::Storage& outputStorage) {
    // variable & id
    int variable = inputStorage.readUnsignedByte();
    std::string id = inputStorage.readString();

    const MSCFModel_CC *model;

    // check variable
    if (variable != ID_LIST && variable != VAR_SPEED && variable != VAR_SPEED_WITHOUT_TRACI && variable != VAR_POSITION && variable != VAR_ANGLE
            && variable != VAR_ROAD_ID && variable != VAR_LANE_ID && variable != VAR_LANE_INDEX
            && variable != VAR_TYPE && variable != VAR_ROUTE_ID && variable != VAR_COLOR
            && variable != VAR_LANEPOSITION
            && variable != VAR_CO2EMISSION && variable != VAR_COEMISSION && variable != VAR_HCEMISSION && variable != VAR_PMXEMISSION
            && variable != VAR_NOXEMISSION && variable != VAR_FUELCONSUMPTION && variable != VAR_NOISEEMISSION
            && variable != VAR_PERSON_NUMBER
            && variable != VAR_EDGE_TRAVELTIME && variable != VAR_EDGE_EFFORT
            && variable != VAR_ROUTE_VALID && variable != VAR_EDGES
            && variable != VAR_SIGNALS
            && variable != VAR_LENGTH && variable != VAR_MAXSPEED && variable != VAR_VEHICLECLASS
            && variable != VAR_SPEED_FACTOR && variable != VAR_SPEED_DEVIATION && variable != VAR_EMISSIONCLASS
            && variable != VAR_WIDTH && variable != VAR_MINGAP && variable != VAR_SHAPECLASS
            && variable != VAR_ACCEL && variable != VAR_DECEL && variable != VAR_IMPERFECTION
            && variable != VAR_TAU && variable != VAR_BEST_LANES && variable != DISTANCE_REQUEST
            && variable != ID_COUNT
            && variable != VAR_GET_SPEED_AND_ACCELERATION
            && variable != VAR_GET_LANES_COUNT
            && variable != VAR_GET_CC_INSTALLED
            && variable != VAR_GET_RADAR_DATA
            && variable != VAR_GET_DISTANCE_TO_END
            && variable != VAR_GET_LANE_CHANGE_ACTION
            && variable != VAR_GET_ACTIVE_CONTROLLER
            && variable != VAR_GET_DISTANCE_FROM_BEGIN
            && variable != VAR_GET_CRASHED
            && variable != VAR_GET_ACC_ACCELERATION
            && variable != VAR_GET_CACC_SPACING
            && variable != VAR_GET_GENERIC_INFORMATION
       ) {
        return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Get Vehicle Variable: unsupported variable specified", outputStorage);
    }
    // begin response building
    tcpip::Storage tempMsg;
    //  response-code, variableID, objectID
    tempMsg.writeUnsignedByte(RESPONSE_GET_VEHICLE_VARIABLE);
    tempMsg.writeUnsignedByte(variable);
    tempMsg.writeString(id);
    // process request
    if (variable == ID_LIST || variable == ID_COUNT) {
        std::vector<std::string> ids;
        MSVehicleControl& c = MSNet::getInstance()->getVehicleControl();
        for (MSVehicleControl::constVehIt i = c.loadedVehBegin(); i != c.loadedVehEnd(); ++i) {
            if ((*i).second->isOnRoad()) {
                ids.push_back((*i).first);
            }
        }
        if (variable == ID_LIST) {
            tempMsg.writeUnsignedByte(TYPE_STRINGLIST);
            tempMsg.writeStringList(ids);
        } else {
            tempMsg.writeUnsignedByte(TYPE_INTEGER);
            tempMsg.writeInt((int) ids.size());
        }
    } else {
        SUMOVehicle* sumoVehicle = MSNet::getInstance()->getVehicleControl().getVehicle(id);
        if (sumoVehicle == 0) {
            return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Vehicle '" + id + "' is not known", outputStorage);
        }
        MSVehicle* v = dynamic_cast<MSVehicle*>(sumoVehicle);
        if (v == 0) {
            return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Vehicle '" + id + "' is not a micro-simulation vehicle", outputStorage);
        }
        const bool onRoad = v->isOnRoad();
        switch (variable) {
            case VAR_SPEED:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? v->getSpeed() : INVALID_DOUBLE_VALUE);
                break;
            case VAR_SPEED_WITHOUT_TRACI:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? v->getSpeedWithoutTraciInfluence() : INVALID_DOUBLE_VALUE);
                break;
            case VAR_POSITION:
                tempMsg.writeUnsignedByte(POSITION_2D);
                tempMsg.writeDouble(onRoad ? v->getPosition().x() : INVALID_DOUBLE_VALUE);
                tempMsg.writeDouble(onRoad ? v->getPosition().y() : INVALID_DOUBLE_VALUE);
                break;
            case VAR_ANGLE:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? v->getAngle() : INVALID_DOUBLE_VALUE);
                break;
            case VAR_ROAD_ID:
                tempMsg.writeUnsignedByte(TYPE_STRING);
                tempMsg.writeString(onRoad ? v->getLane()->getEdge().getID() : "");
                break;
            case VAR_LANE_ID:
                tempMsg.writeUnsignedByte(TYPE_STRING);
                tempMsg.writeString(onRoad ? v->getLane()->getID() : "");
                break;
            case VAR_LANE_INDEX:
                tempMsg.writeUnsignedByte(TYPE_INTEGER);
                if (onRoad) {
                    const std::vector<MSLane*>& lanes = v->getLane()->getEdge().getLanes();
                    tempMsg.writeInt((int)std::distance(lanes.begin(), std::find(lanes.begin(), lanes.end(), v->getLane())));
                } else {
                    tempMsg.writeInt(INVALID_INT_VALUE);
                }
                break;
            case VAR_TYPE:
                tempMsg.writeUnsignedByte(TYPE_STRING);
                tempMsg.writeString(v->getVehicleType().getID());
                break;
            case VAR_ROUTE_ID:
                tempMsg.writeUnsignedByte(TYPE_STRING);
                tempMsg.writeString(v->getRoute().getID());
                break;
            case VAR_COLOR:
                tempMsg.writeUnsignedByte(TYPE_COLOR);
                tempMsg.writeUnsignedByte(v->getParameter().color.red());
                tempMsg.writeUnsignedByte(v->getParameter().color.green());
                tempMsg.writeUnsignedByte(v->getParameter().color.blue());
                tempMsg.writeUnsignedByte(v->getParameter().color.alpha());
                break;
            case VAR_LANEPOSITION:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? v->getPositionOnLane() : INVALID_DOUBLE_VALUE);
                break;
            case VAR_CO2EMISSION:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? HelpersHBEFA::computeCO2(v->getVehicleType().getEmissionClass(), v->getSpeed(), v->getAcceleration()) : INVALID_DOUBLE_VALUE);
                break;
            case VAR_COEMISSION:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? HelpersHBEFA::computeCO(v->getVehicleType().getEmissionClass(), v->getSpeed(), v->getAcceleration()) : INVALID_DOUBLE_VALUE);
                break;
            case VAR_HCEMISSION:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? HelpersHBEFA::computeHC(v->getVehicleType().getEmissionClass(), v->getSpeed(), v->getAcceleration()) : INVALID_DOUBLE_VALUE);
                break;
            case VAR_PMXEMISSION:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? HelpersHBEFA::computePMx(v->getVehicleType().getEmissionClass(), v->getSpeed(), v->getAcceleration()) : INVALID_DOUBLE_VALUE);
                break;
            case VAR_NOXEMISSION:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? HelpersHBEFA::computeNOx(v->getVehicleType().getEmissionClass(), v->getSpeed(), v->getAcceleration()) : INVALID_DOUBLE_VALUE);
                break;
            case VAR_FUELCONSUMPTION:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? HelpersHBEFA::computeFuel(v->getVehicleType().getEmissionClass(), v->getSpeed(), v->getAcceleration()) : INVALID_DOUBLE_VALUE);
                break;
            case VAR_NOISEEMISSION:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? HelpersHarmonoise::computeNoise(v->getVehicleType().getEmissionClass(), v->getSpeed(), v->getAcceleration()) : INVALID_DOUBLE_VALUE);
                break;
            case VAR_PERSON_NUMBER:
                tempMsg.writeUnsignedByte(TYPE_INTEGER);
                tempMsg.writeInt(v->getPersonNumber());
                break;
            case VAR_EDGE_TRAVELTIME: {
                if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Retrieval of travel time requires a compound object.", outputStorage);
                }
                if (inputStorage.readInt() != 2) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Retrieval of travel time requires time, and edge as parameter.", outputStorage);
                }
                // time
                SUMOTime time = 0;
                if (!server.readTypeCheckingInt(inputStorage, time)) {
                    return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Retrieval of travel time requires the referenced time as first parameter.", outputStorage);
                }
                // edge
                std::string edgeID;
                if (!server.readTypeCheckingString(inputStorage, edgeID)) {
                    return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Retrieval of travel time requires the referenced edge as second parameter.", outputStorage);
                }
                MSEdge* edge = MSEdge::dictionary(edgeID);
                if (edge == 0) {
                    return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Referenced edge '" + edgeID + "' is not known.", outputStorage);
                }
                // retrieve
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                SUMOReal value;
                if (!v->getWeightsStorage().retrieveExistingTravelTime(edge, 0, time, value)) {
                    tempMsg.writeDouble(INVALID_DOUBLE_VALUE);
                } else {
                    tempMsg.writeDouble(value);
                }

            }
            break;
            case VAR_EDGE_EFFORT: {
                if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Retrieval of travel time requires a compound object.", outputStorage);
                }
                if (inputStorage.readInt() != 2) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Retrieval of travel time requires time, and edge as parameter.", outputStorage);
                }
                // time
                SUMOTime time = 0;
                if (!server.readTypeCheckingInt(inputStorage, time)) {
                    return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Retrieval of effort requires the referenced time as first parameter.", outputStorage);
                }
                // edge
                std::string edgeID;
                if (!server.readTypeCheckingString(inputStorage, edgeID)) {
                    return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Retrieval of effort requires the referenced edge as second parameter.", outputStorage);
                }
                MSEdge* edge = MSEdge::dictionary(edgeID);
                if (edge == 0) {
                    return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Referenced edge '" + edgeID + "' is not known.", outputStorage);
                }
                // retrieve
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                SUMOReal value;
                if (!v->getWeightsStorage().retrieveExistingEffort(edge, 0, time, value)) {
                    tempMsg.writeDouble(INVALID_DOUBLE_VALUE);
                } else {
                    tempMsg.writeDouble(value);
                }

            }
            break;
            case VAR_ROUTE_VALID: {
                std::string msg;
                tempMsg.writeUnsignedByte(TYPE_UBYTE);
                tempMsg.writeUnsignedByte(v->hasValidRoute(msg));
            }
            break;
            case VAR_EDGES: {
                const MSRoute& r = v->getRoute();
                tempMsg.writeUnsignedByte(TYPE_STRINGLIST);
                tempMsg.writeInt(r.size());
                for (MSRouteIterator i = r.begin(); i != r.end(); ++i) {
                    tempMsg.writeString((*i)->getID());
                }
            }
            break;
            case VAR_SIGNALS:
                tempMsg.writeUnsignedByte(TYPE_INTEGER);
                tempMsg.writeInt(v->getSignals());
                break;
            case VAR_BEST_LANES: {
                tempMsg.writeUnsignedByte(TYPE_COMPOUND);
                tcpip::Storage tempContent;
                unsigned int cnt = 0;
                tempContent.writeUnsignedByte(TYPE_INTEGER);
                const std::vector<MSVehicle::LaneQ>& bestLanes = onRoad ? v->getBestLanes() : std::vector<MSVehicle::LaneQ>();
                tempContent.writeInt((int) bestLanes.size());
                ++cnt;
                for (std::vector<MSVehicle::LaneQ>::const_iterator i = bestLanes.begin(); i != bestLanes.end(); ++i) {
                    const MSVehicle::LaneQ& lq = *i;
                    tempContent.writeUnsignedByte(TYPE_STRING);
                    tempContent.writeString(lq.lane->getID());
                    ++cnt;
                    tempContent.writeUnsignedByte(TYPE_DOUBLE);
                    tempContent.writeDouble(lq.length);
                    ++cnt;
                    tempContent.writeUnsignedByte(TYPE_DOUBLE);
                    tempContent.writeDouble(lq.nextOccupation);
                    ++cnt;
                    tempContent.writeUnsignedByte(TYPE_BYTE);
                    tempContent.writeByte(lq.bestLaneOffset);
                    ++cnt;
                    tempContent.writeUnsignedByte(TYPE_UBYTE);
                    lq.allowsContinuation ? tempContent.writeUnsignedByte(1) : tempContent.writeUnsignedByte(0);
                    ++cnt;
                    std::vector<std::string> bestContIDs;
                    for (std::vector<MSLane*>::const_iterator j = lq.bestContinuations.begin(); j != lq.bestContinuations.end(); ++j) {
                        bestContIDs.push_back((*j)->getID());
                    }
                    tempContent.writeUnsignedByte(TYPE_STRINGLIST);
                    tempContent.writeStringList(bestContIDs);
                    ++cnt;
                }
                tempMsg.writeInt((int) cnt);
                tempMsg.writeStorage(tempContent);
            }
            break;
            case DISTANCE_REQUEST:
                if (!commandDistanceRequest(server, inputStorage, tempMsg, v)) {
                    return false;
                }
                break;
            default:

        case VAR_GET_SPEED_AND_ACCELERATION:
        {
            SUMOReal speed;
            SUMOReal acceleration;
            SUMOReal controllerAcceleration;
            Position position;
            SUMOReal time;

            model = dynamic_cast<const MSCFModel_CC *>(&v->getCarFollowModel());
            assert(model);

            model->getVehicleInformation((const MSVehicle *)v, speed, acceleration, controllerAcceleration, position, time);

            tempMsg.writeUnsignedByte(TYPE_DOUBLE);
            tempMsg.writeDouble(speed);

            tempMsg.writeUnsignedByte(TYPE_DOUBLE);
            tempMsg.writeDouble(acceleration);

            tempMsg.writeDouble(controllerAcceleration);

            tempMsg.writeDouble(position.x());
            tempMsg.writeDouble(position.y());
            tempMsg.writeDouble(time);

            break;
        }
        case VAR_GET_LANES_COUNT:

            tempMsg.writeUnsignedByte(TYPE_INTEGER);
            if (onRoad) {
                const std::vector<MSLane*> &lanes = v->getLane()->getEdge().getLanes();
                tempMsg.writeInt((int)lanes.size());
            } else {
                tempMsg.writeInt(INVALID_INT_VALUE);
            }
            break;

        case VAR_GET_CC_INSTALLED:

            model = dynamic_cast<const MSCFModel_CC *>(&v->getCarFollowModel());

            tempMsg.writeUnsignedByte(TYPE_UBYTE);
            tempMsg.writeUnsignedByte(model != 0);

            break;

        case VAR_GET_RADAR_DATA:

            SUMOReal distance, relativeSpeed;

            model = dynamic_cast<const MSCFModel_CC *>(&v->getCarFollowModel());
            assert(model);

            model->getRadarMeasurements(v, distance, relativeSpeed);

            tempMsg.writeUnsignedByte(TYPE_DOUBLE);
            tempMsg.writeDouble(distance);
            tempMsg.writeUnsignedByte(TYPE_DOUBLE);
            tempMsg.writeDouble(relativeSpeed);

            break;

        case VAR_GET_DISTANCE_TO_END:

            //route of the vehicle
            const MSRoute *route;
            //edge the vehicle is currently traveling on
            const MSEdge *currentEdge;
            //last edge of the route of this vehicle
            const MSEdge *lastEdge;
            //current position of the vehicle on the edge its traveling in
            double positionOnEdge;
            //distance to trip end using
            double distanceToEnd;

            route = &v->getRoute();
            currentEdge = v->getEdge();
            lastEdge = route->getEdges().back();
            positionOnEdge = v->getPositionOnLane();
            distanceToEnd = route->getDistanceBetween(positionOnEdge, lastEdge->getLanes()[0]->getLength(), currentEdge, lastEdge);

            //write message back
            tempMsg.writeUnsignedByte(TYPE_DOUBLE);
            tempMsg.writeDouble(distanceToEnd);

            break;
            
        case VAR_GET_DISTANCE_FROM_BEGIN:
        {

            //route of the vehicle
            const MSRoute *route;
            //edge the vehicle is currently traveling on
            const MSEdge *currentEdge;
            //last edge of the route of this vehicle
            const MSEdge *firstEdge;
            //current position of the vehicle on the edge its traveling in
            double positionOnEdge;
            //distance to trip end using
            double distanceFromBegin;

            route = &v->getRoute();
            currentEdge = v->getEdge();
            firstEdge = route->getEdges().front();
            positionOnEdge = v->getPositionOnLane();
            distanceFromBegin = route->getDistanceBetween(0, positionOnEdge, firstEdge, currentEdge);

            //write message back
            tempMsg.writeUnsignedByte(TYPE_DOUBLE);
            tempMsg.writeDouble(distanceFromBegin);

            break;
        }

        case VAR_GET_CRASHED:

            model = dynamic_cast<const MSCFModel_CC *>(&v->getCarFollowModel());
            assert(model);

            tempMsg.writeUnsignedByte(TYPE_INTEGER);
            tempMsg.writeInt(model->isCrashed(v));

            break;


        case VAR_GET_LANE_CHANGE_ACTION:

            model = dynamic_cast<const MSCFModel_CC *>(&v->getCarFollowModel());
            assert(model);

            tempMsg.writeUnsignedByte(TYPE_INTEGER);
            tempMsg.writeInt(model->getLaneChangeAction(v));

            break;

        case VAR_GET_ACTIVE_CONTROLLER:

            model = dynamic_cast<const MSCFModel_CC *>(&v->getCarFollowModel());
            assert(model);

            tempMsg.writeUnsignedByte(TYPE_INTEGER);
            tempMsg.writeInt(model->getActiveController(v));

            break;

        case VAR_GET_ACC_ACCELERATION: {

            model = dynamic_cast<const MSCFModel_CC *>(&v->getCarFollowModel());
            assert(model);

            tempMsg.writeDouble(model->getACCAcceleration(v));

            break;


        }
        case VAR_GET_CACC_SPACING: {
            model = dynamic_cast<const MSCFModel_CC *>(&v->getCarFollowModel());
            assert(model);

            tempMsg.writeDouble(model->getCACCConstantSpacing(v));

            break;
        }
        case VAR_GET_GENERIC_INFORMATION: {

            const MSCFModel_CC * model;
            model = dynamic_cast<const MSCFModel_CC*>(&v->getVehicleType().getCarFollowModel());
            assert(model);

            //request info
            struct Plexe::CCDataHeader request;
            //response info
            struct Plexe::CCDataHeader response;
            //request params
            void *reqParams = 0;
            //content to be returned, supposing 4KB is big enough :)
            unsigned char content[4096];
            //actual size of the content
            int size;

            //get requested information type
            inputStorage.readBuffer((unsigned char *)&request, sizeof(struct Plexe::CCDataHeader));

            //if there are parameters, get them
            if (request.size != 0) {
                reqParams = malloc(request.size);
                inputStorage.readBuffer((unsigned char *)reqParams, request.size);
            }

            //request data to model
            size = model->getGenericInformation((const MSVehicle *)v, request, reqParams, content);
            //write response header
            response.type = request.type;
            response.size = size;
            tempMsg.writeBuffer((unsigned char *)&response, sizeof(struct Plexe::CCDataHeader));
            //write data into response message
            tempMsg.writeBuffer(content, size);

            if (reqParams) {
                free(reqParams);
            }

            break;

        }

        TraCIServerAPI_VehicleType::getVariable(variable, v->getVehicleType(), tempMsg);
        break;
        }

    }
    server.writeStatusCmd(CMD_GET_VEHICLE_VARIABLE, RTYPE_OK, "", outputStorage);
    server.writeResponseWithLength(outputStorage, tempMsg);
    return true;
}


bool
TraCIServerAPI_Vehicle::processSet(TraCIServer& server, tcpip::Storage& inputStorage,
                                   tcpip::Storage& outputStorage) {
    std::string warning = ""; // additional description for response
    // variable
    int variable = inputStorage.readUnsignedByte();
    if (variable != CMD_STOP && variable != CMD_CHANGELANE
            && variable != CMD_SLOWDOWN && variable != CMD_CHANGETARGET
            && variable != VAR_ROUTE_ID && variable != VAR_ROUTE
            && variable != VAR_EDGE_TRAVELTIME && variable != VAR_EDGE_EFFORT
            && variable != CMD_REROUTE_TRAVELTIME && variable != CMD_REROUTE_EFFORT
            && variable != VAR_SIGNALS && variable != VAR_MOVE_TO
            && variable != VAR_LENGTH && variable != VAR_MAXSPEED && variable != VAR_VEHICLECLASS
            && variable != VAR_SPEED_FACTOR && variable != VAR_SPEED_DEVIATION && variable != VAR_EMISSIONCLASS
            && variable != VAR_WIDTH && variable != VAR_MINGAP && variable != VAR_SHAPECLASS
            && variable != VAR_ACCEL && variable != VAR_DECEL && variable != VAR_IMPERFECTION
            && variable != VAR_TAU
            && variable != VAR_SPEED && variable != VAR_SPEEDSETMODE && variable != VAR_COLOR
            && variable != ADD && variable != REMOVE
            && variable != VAR_MOVE_TO_VTD
            && variable != VAR_SET_LEADER_SPEED_AND_ACCELERATION
            && variable != VAR_SET_ACTIVE_CONTROLLER
            && variable != VAR_SET_CC_DESIRED_SPEED
            && variable != VAR_SET_LANE_CHANGE_ACTION
            && variable != VAR_SET_CONTROLLER_FAKE_DATA
            && variable != VAR_SET_PRECEDING_SPEED_AND_ACCELERATION
            && variable != VAR_SET_FIXED_LANE
            && variable != VAR_SET_ACC_HEADWAY_TIME
            && variable != VAR_SET_FIXED_ACCELERATION
            && variable != VAR_SET_CACC_SPACING
            && variable != VAR_SET_GENERIC_INFORMATION
       ) {
        return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Change Vehicle State: unsupported variable specified", outputStorage);
    }
    // id
    std::string id = inputStorage.readString();
    const bool shouldExist = variable != ADD;
    SUMOVehicle* sumoVehicle = MSNet::getInstance()->getVehicleControl().getVehicle(id);
    if (sumoVehicle == 0) {
        if (shouldExist) {
            return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Vehicle '" + id + "' is not known", outputStorage);
        }
    }
    MSVehicle* v = dynamic_cast<MSVehicle*>(sumoVehicle);
    if (v == 0 && shouldExist) {
        return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Vehicle '" + id + "' is not a micro-simulation vehicle", outputStorage);
    }
    switch (variable) {
        case CMD_STOP: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Stop needs a compound object description.", outputStorage);
            }
            if (inputStorage.readInt() != 4) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Stop needs a compound object description of four items.", outputStorage);
            }
            // read road map position
            std::string roadId;
            if (!server.readTypeCheckingString(inputStorage, roadId)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The first stop parameter must be the edge id given as a string.", outputStorage);
            }
            double pos = 0;
            if (!server.readTypeCheckingDouble(inputStorage, pos)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The second stop parameter must be the position along the edge given as a double.", outputStorage);
            }
            int laneIndex = 0;
            if (!server.readTypeCheckingByte(inputStorage, laneIndex)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The third stop parameter must be the lane index given as a byte.", outputStorage);
            }
            // waitTime
            SUMOTime waitTime = 0;
            if (!server.readTypeCheckingInt(inputStorage, waitTime)) {
                return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "The fourth stop parameter must be the waiting time given as an integer.", outputStorage);
            }
            // check
            if (pos < 0) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Position on lane must not be negative", outputStorage);
            }
            // get the actual lane that is referenced by laneIndex
            MSEdge* road = MSEdge::dictionary(roadId);
            if (road == 0) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Unable to retrieve road with given id", outputStorage);
            }
            const std::vector<MSLane*>& allLanes = road->getLanes();
            if ((laneIndex < 0) || laneIndex >= (int)(allLanes.size())) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "No lane existing with such id on the given road", outputStorage);
            }
            // Forward command to vehicle
            if (!v->addTraciStop(allLanes[laneIndex], pos, 0, waitTime)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Vehicle is too close or behind the stop on " + allLanes[laneIndex]->getID(), outputStorage);
            }
        }
        break;
        case CMD_CHANGELANE: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Lane change needs a compound object description.", outputStorage);
            }
            if (inputStorage.readInt() != 2) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Lane change needs a compound object description of two items.", outputStorage);
            }
            // Lane ID
            int laneIndex = 0;
            if (!server.readTypeCheckingByte(inputStorage, laneIndex)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The first lane change parameter must be the lane index given as a byte.", outputStorage);
            }
            // stickyTime
            SUMOTime stickyTime = 0;
            if (!server.readTypeCheckingInt(inputStorage, stickyTime)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The second lane change parameter must be the duration given as an integer.", outputStorage);
            }
            if ((laneIndex < 0) || (laneIndex >= (int)(v->getEdge()->getLanes().size()))) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "No lane existing with given id on the current road", outputStorage);
            }
            // Forward command to vehicle
            std::vector<std::pair<SUMOTime, unsigned int> > laneTimeLine;
            laneTimeLine.push_back(std::make_pair(MSNet::getInstance()->getCurrentTimeStep(), laneIndex));
            laneTimeLine.push_back(std::make_pair(MSNet::getInstance()->getCurrentTimeStep() + stickyTime, laneIndex));
            v->getInfluencer().setLaneTimeLine(laneTimeLine);
            MSVehicle::ChangeRequest req = v->getInfluencer().checkForLaneChanges(MSNet::getInstance()->getCurrentTimeStep(),
                                           *v->getEdge(), v->getLaneIndex());
            v->getLaneChangeModel().requestLaneChange(req);
        }
        break;
        case CMD_SLOWDOWN: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Slow down needs a compound object description.", outputStorage);
            }
            if (inputStorage.readInt() != 2) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Slow down needs a compound object description of two items.", outputStorage);
            }
            double newSpeed = 0;
            if (!server.readTypeCheckingDouble(inputStorage, newSpeed)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The first slow down parameter must be the speed given as a double.", outputStorage);
            }
            if (newSpeed < 0) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Speed must not be negative", outputStorage);
            }
            SUMOTime duration = 0;
            if (!server.readTypeCheckingInt(inputStorage, duration)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The second slow down parameter must be the duration given as an integer.", outputStorage);
            }
            if (duration < 0) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Invalid time interval", outputStorage);
            }
            std::vector<std::pair<SUMOTime, SUMOReal> > speedTimeLine;
            speedTimeLine.push_back(std::make_pair(MSNet::getInstance()->getCurrentTimeStep(), v->getSpeed()));
            speedTimeLine.push_back(std::make_pair(MSNet::getInstance()->getCurrentTimeStep() + duration, newSpeed));
            v->getInfluencer().setSpeedTimeLine(speedTimeLine);
        }
        break;
        case CMD_CHANGETARGET: {
            std::string edgeID;
            if (!server.readTypeCheckingString(inputStorage, edgeID)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Change target requires a string containing the id of the new destination edge as parameter.", outputStorage);
            }
            const MSEdge* destEdge = MSEdge::dictionary(edgeID);
            if (destEdge == 0) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Can not retrieve road with ID " + edgeID, outputStorage);
            }
            // build a new route between the vehicle's current edge and destination edge
            MSEdgeVector newRoute;
            const MSEdge* currentEdge = v->getEdge();
            MSNet::getInstance()->getRouterTT().compute(
                currentEdge, destEdge, (const MSVehicle * const) v, MSNet::getInstance()->getCurrentTimeStep(), newRoute);
            // replace the vehicle's route by the new one
            if (!v->replaceRouteEdges(newRoute)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Route replacement failed for " + v->getID(), outputStorage);
            }
        }
        break;
        case VAR_ROUTE_ID: {
            std::string rid;
            if (!server.readTypeCheckingString(inputStorage, rid)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The route id must be given as a string.", outputStorage);
            }
            const MSRoute* r = MSRoute::dictionary(rid);
            if (r == 0) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The route '" + rid + "' is not known.", outputStorage);
            }
            if (!v->replaceRoute(r)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Route replacement failed for " + v->getID(), outputStorage);
            }
        }
        break;
        case VAR_ROUTE: {
            std::vector<std::string> edgeIDs;
            if (!server.readTypeCheckingStringList(inputStorage, edgeIDs)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "A route must be defined as a list of edge ids.", outputStorage);
            }
            std::vector<const MSEdge*> edges;
            MSEdge::parseEdgesList(edgeIDs, edges, "<unknown>");
            if (!v->replaceRouteEdges(edges)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Route replacement failed for " + v->getID(), outputStorage);
            }
        }
        break;
        case VAR_EDGE_TRAVELTIME: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting travel time requires a compound object.", outputStorage);
            }
            int parameterCount = inputStorage.readInt();
            if (parameterCount == 4) {
                // begin time
                SUMOTime begTime = 0, endTime = 0;
                if (!server.readTypeCheckingInt(inputStorage, begTime)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting travel time using 4 parameters requires the begin time as first parameter.", outputStorage);
                }
                // begin time
                if (!server.readTypeCheckingInt(inputStorage, endTime)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting travel time using 4 parameters requires the end time as second parameter.", outputStorage);
                }
                // edge
                std::string edgeID;
                if (!server.readTypeCheckingString(inputStorage, edgeID)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting travel time using 4 parameters requires the referenced edge as third parameter.", outputStorage);
                }
                MSEdge* edge = MSEdge::dictionary(edgeID);
                if (edge == 0) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Referenced edge '" + edgeID + "' is not known.", outputStorage);
                }
                // value
                double value = 0;
                if (!server.readTypeCheckingDouble(inputStorage, value)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting travel time using 4 parameters requires the travel time as double as fourth parameter.", outputStorage);
                }
                // retrieve
                v->getWeightsStorage().addTravelTime(edge, begTime, endTime, value);
            } else if (parameterCount == 2) {
                // edge
                std::string edgeID;
                if (!server.readTypeCheckingString(inputStorage, edgeID)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting travel time using 2 parameters requires the referenced edge as first parameter.", outputStorage);
                }
                MSEdge* edge = MSEdge::dictionary(edgeID);
                if (edge == 0) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Referenced edge '" + edgeID + "' is not known.", outputStorage);
                }
                // value
                double value = 0;
                if (!server.readTypeCheckingDouble(inputStorage, value)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting travel time using 2 parameters requires the travel time as second parameter.", outputStorage);
                }
                // retrieve
                while (v->getWeightsStorage().knowsTravelTime(edge)) {
                    v->getWeightsStorage().removeTravelTime(edge);
                }
                v->getWeightsStorage().addTravelTime(edge, 0, SUMOTime_MAX, value);
            } else if (parameterCount == 1) {
                // edge
                std::string edgeID;
                if (!server.readTypeCheckingString(inputStorage, edgeID)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting travel time using 1 parameter requires the referenced edge as first parameter.", outputStorage);
                }
                MSEdge* edge = MSEdge::dictionary(edgeID);
                if (edge == 0) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Referenced edge '" + edgeID + "' is not known.", outputStorage);
                }
                // retrieve
                while (v->getWeightsStorage().knowsTravelTime(edge)) {
                    v->getWeightsStorage().removeTravelTime(edge);
                }
            } else {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting travel time requires 1, 2, or 4 parameters.", outputStorage);
            }
        }
        break;
        case VAR_EDGE_EFFORT: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting effort requires a compound object.", outputStorage);
            }
            int parameterCount = inputStorage.readInt();
            if (parameterCount == 4) {
                // begin time
                SUMOTime begTime = 0, endTime = 0;
                if (!server.readTypeCheckingInt(inputStorage, begTime)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting effort using 4 parameters requires the begin time as first parameter.", outputStorage);
                }
                // begin time
                if (!server.readTypeCheckingInt(inputStorage, endTime)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting effort using 4 parameters requires the end time as second parameter.", outputStorage);
                }
                // edge
                std::string edgeID;
                if (!server.readTypeCheckingString(inputStorage, edgeID)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting effort using 4 parameters requires the referenced edge as third parameter.", outputStorage);
                }
                MSEdge* edge = MSEdge::dictionary(edgeID);
                if (edge == 0) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Referenced edge '" + edgeID + "' is not known.", outputStorage);
                }
                // value
                double value = 0;
                if (!server.readTypeCheckingDouble(inputStorage, value)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting effort using 4 parameters requires the travel time as fourth parameter.", outputStorage);
                }
                // retrieve
                v->getWeightsStorage().addEffort(edge, begTime, endTime, value);
            } else if (parameterCount == 2) {
                // edge
                std::string edgeID;
                if (!server.readTypeCheckingString(inputStorage, edgeID)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting effort using 2 parameters requires the referenced edge as first parameter.", outputStorage);
                }
                MSEdge* edge = MSEdge::dictionary(edgeID);
                if (edge == 0) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Referenced edge '" + edgeID + "' is not known.", outputStorage);
                }
                // value
                double value = 0;
                if (!server.readTypeCheckingDouble(inputStorage, value)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting effort using 2 parameters requires the travel time as second parameter.", outputStorage);
                }
                // retrieve
                while (v->getWeightsStorage().knowsEffort(edge)) {
                    v->getWeightsStorage().removeEffort(edge);
                }
                v->getWeightsStorage().addEffort(edge, 0, SUMOTime_MAX, value);
            } else if (parameterCount == 1) {
                // edge
                std::string edgeID;
                if (!server.readTypeCheckingString(inputStorage, edgeID)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting effort using 1 parameter requires the referenced edge as first parameter.", outputStorage);
                }
                MSEdge* edge = MSEdge::dictionary(edgeID);
                if (edge == 0) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Referenced edge '" + edgeID + "' is not known.", outputStorage);
                }
                // retrieve
                while (v->getWeightsStorage().knowsEffort(edge)) {
                    v->getWeightsStorage().removeEffort(edge);
                }
            } else {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting effort requires 1, 2, or 4 parameters.", outputStorage);
            }
        }
        break;
        case CMD_REROUTE_TRAVELTIME: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Rerouting requires a compound object.", outputStorage);
            }
            if (inputStorage.readInt() != 0) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Rerouting should obtain an empty compound object.", outputStorage);
            }
            v->reroute(MSNet::getInstance()->getCurrentTimeStep(), MSNet::getInstance()->getRouterTT());
        }
        break;
        case CMD_REROUTE_EFFORT: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Rerouting requires a compound object.", outputStorage);
            }
            if (inputStorage.readInt() != 0) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Rerouting should obtain an empty compound object.", outputStorage);
            }
            v->reroute(MSNet::getInstance()->getCurrentTimeStep(), MSNet::getInstance()->getRouterEffort());
        }
        break;
        case VAR_SIGNALS: {
            int signals = 0;
            if (!server.readTypeCheckingInt(inputStorage, signals)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting signals requires an integer.", outputStorage);
            }
            v->switchOffSignal(0x0fffffff);
            v->switchOnSignal(signals);
        }
        break;
        case VAR_MOVE_TO: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting position requires a compound object.", outputStorage);
            }
            if (inputStorage.readInt() != 2) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting position should obtain the lane id and the position.", outputStorage);
            }
            // lane ID
            std::string laneID;
            if (!server.readTypeCheckingString(inputStorage, laneID)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The first parameter for setting a position must be the lane ID given as a string.", outputStorage);
            }
            // position on lane
            double position = 0;
            if (!server.readTypeCheckingDouble(inputStorage, position)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The second parameter for setting a position must be the position given as a double.", outputStorage);
            }
            // process
            MSLane* l = MSLane::dictionary(laneID);
            if (l == 0) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Unknown lane '" + laneID + "'.", outputStorage);
            }
            MSEdge& destinationEdge = l->getEdge();
            if (!v->willPass(&destinationEdge)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Vehicle '" + laneID + "' may be set onto an edge to pass only.", outputStorage);
            }
            v->onRemovalFromNet(MSMoveReminder::NOTIFICATION_TELEPORT);
            v->getLane()->removeVehicle(v);
            while (v->getEdge() != &destinationEdge) {
                const MSEdge* nextEdge = v->succEdge(1);
                // let the vehicle move to the next edge
                if (v->enterLaneAtMove(nextEdge->getLanes()[0], true)) {
                    MSNet::getInstance()->getVehicleControl().scheduleVehicleRemoval(v);
                    continue;
                }
            }
            l->forceVehicleInsertion(v, position);
        }
        break;
        case VAR_SPEED: {
            double speed = 0;
            if (!server.readTypeCheckingDouble(inputStorage, speed)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting speed requires a double.", outputStorage);
            }
            std::vector<std::pair<SUMOTime, SUMOReal> > speedTimeLine;
            if (speed >= 0) {
                speedTimeLine.push_back(std::make_pair(MSNet::getInstance()->getCurrentTimeStep(), speed));
                speedTimeLine.push_back(std::make_pair(SUMOTime_MAX, speed));
            }
            v->getInfluencer().setSpeedTimeLine(speedTimeLine);
        }
        break;
        case VAR_SPEEDSETMODE: {
            int speedMode = 0;
            if (!server.readTypeCheckingInt(inputStorage, speedMode)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting speed mode requires an integer.", outputStorage);
            }
            v->getInfluencer().setConsiderSafeVelocity((speedMode & 1) != 0);
            v->getInfluencer().setConsiderMaxAcceleration((speedMode & 2) != 0);
            v->getInfluencer().setConsiderMaxDeceleration((speedMode & 4) != 0);
        }
        break;
        case VAR_COLOR: {
            RGBColor col;
            if (!server.readTypeCheckingColor(inputStorage, col)) {
                return server.writeErrorStatusCmd(CMD_SET_POLYGON_VARIABLE, "The color must be given using the according type.", outputStorage);
            }
            v->getParameter().color.set(col.red(), col.green(), col.blue(), col.alpha());
            v->getParameter().setParameter |= VEHPARS_COLOR_SET;
        }
        break;
        case ADD: {
            if (v != 0) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The vehicle " + id + " to add already exists.", outputStorage);
            }
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Adding a vehicle requires a compound object.", outputStorage);
            }
            if (inputStorage.readInt() != 6) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Adding a vehicle needs six parameters.", outputStorage);
            }
            SUMOVehicleParameter vehicleParams;
            vehicleParams.id = id;

            std::string vTypeID;
            if (!server.readTypeCheckingString(inputStorage, vTypeID)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "First parameter (type) requires a string.", outputStorage);
            }
            MSVehicleType* vehicleType = MSNet::getInstance()->getVehicleControl().getVType(vTypeID);
            if (!vehicleType) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Invalid type '" + vTypeID + "' for vehicle '" + id + "'", outputStorage);
            }

            std::string routeID;
            if (!server.readTypeCheckingString(inputStorage, routeID)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Second parameter (route) requires a string.", outputStorage);
            }
            const MSRoute* route = MSRoute::dictionary(routeID);
            if (!route) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Invalid route '" + routeID + "' for vehicle: '" + id + "'", outputStorage);
            }

            if (!server.readTypeCheckingInt(inputStorage, vehicleParams.depart)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Third parameter (depart) requires an integer.", outputStorage);
            }
            if (vehicleParams.depart < 0) {
                const int proc = static_cast<int>(-vehicleParams.depart);
                if (proc >= static_cast<int>(DEPART_DEF_MAX)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Invalid departure time.", outputStorage);
                }
                vehicleParams.departProcedure = (DepartDefinition)proc;
            }

            double pos;
            if (!server.readTypeCheckingDouble(inputStorage, pos)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Fourth parameter (position) requires a double.", outputStorage);
            }
            vehicleParams.departPos = pos;
            if (vehicleParams.departPos < 0) {
                const int proc = static_cast<int>(-vehicleParams.departPos);
                if (proc >= static_cast<int>(DEPART_POS_DEF_MAX)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Invalid departure position.", outputStorage);
                }
                vehicleParams.departPosProcedure = (DepartPosDefinition)proc;
            } else {
                vehicleParams.departPosProcedure = DEPART_POS_GIVEN;
            }

            double speed;
            if (!server.readTypeCheckingDouble(inputStorage, speed)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Fifth parameter (speed) requires a double.", outputStorage);
            }
            vehicleParams.departSpeed = speed;
            if (vehicleParams.departSpeed < 0) {
                const int proc = static_cast<int>(-vehicleParams.departSpeed);
                if (proc >= static_cast<int>(DEPART_SPEED_DEF_MAX)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Invalid departure speed.", outputStorage);
                }
                vehicleParams.departSpeedProcedure = (DepartSpeedDefinition)proc;
            } else {
                vehicleParams.departSpeedProcedure = DEPART_SPEED_GIVEN;
            }

            if (!server.readTypeCheckingByte(inputStorage, vehicleParams.departLane)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Sixth parameter (lane) requires a byte.", outputStorage);
            }

            if (vehicleParams.departLane < 0) {
                const int proc = static_cast<int>(-vehicleParams.departLane);
                if (proc >= static_cast<int>(DEPART_LANE_DEF_MAX)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Invalid departure lane.", outputStorage);
                }
                vehicleParams.departLaneProcedure = (DepartLaneDefinition)proc;
            } else {
                vehicleParams.departLaneProcedure = DEPART_LANE_GIVEN;
            }

            SUMOVehicleParameter* params = new SUMOVehicleParameter();
            *params = vehicleParams;
            try {
                SUMOVehicle* vehicle = MSNet::getInstance()->getVehicleControl().buildVehicle(params, route, vehicleType);
                MSNet::getInstance()->getVehicleControl().addVehicle(vehicleParams.id, vehicle);
                MSNet::getInstance()->getInsertionControl().add(vehicle);
            } catch (ProcessError& e) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, e.what(), outputStorage);
            }
        }
        break;
        case REMOVE: {
            int why = 0;
            if (!server.readTypeCheckingByte(inputStorage, why)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Removing a vehicle requires a byte.", outputStorage);
            }
            MSMoveReminder::Notification n = MSMoveReminder::NOTIFICATION_ARRIVED;
            switch (why) {
                case REMOVE_TELEPORT:
                    n = MSMoveReminder::NOTIFICATION_TELEPORT;
                    break;
                case REMOVE_PARKING:
                    n = MSMoveReminder::NOTIFICATION_PARKING;
                    break;
                case REMOVE_ARRIVED:
                    n = MSMoveReminder::NOTIFICATION_ARRIVED;
                    break;
                case REMOVE_VAPORIZED:
                    n = MSMoveReminder::NOTIFICATION_VAPORIZED;
                    break;
                case REMOVE_TELEPORT_ARRIVED:
                    n = MSMoveReminder::NOTIFICATION_TELEPORT_ARRIVED;
                    break;
                default:
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Unknown removal status.", outputStorage);
            }
            if (v->hasDeparted()) {
                v->onRemovalFromNet(n);
                if (v->getLane() != 0) {
                    v->getLane()->removeVehicle(v);
                }
                MSNet::getInstance()->getVehicleControl().scheduleVehicleRemoval(v);
            }
        }
        break;
        case VAR_MOVE_TO_VTD: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting VTD vehicle requires a compound object.", outputStorage);
            }
            if (inputStorage.readInt() != 4) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting VTD vehicle should obtain: edgeID, lane, x, y.", outputStorage);
            }
            // edge ID
            std::string edgeID;
            if (!server.readTypeCheckingString(inputStorage, edgeID)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The first parameter for setting a VTD vehicle must be the edge ID given as a string.", outputStorage);
            }
            // lane index
            int laneNum = 0;
            if (!server.readTypeCheckingInt(inputStorage, laneNum)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The second parameter for setting a VTD vehicle must be lane given as an int.", outputStorage);
            }
            // x
            double x = 0, y = 0;
            if (!server.readTypeCheckingDouble(inputStorage, x)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The third parameter for setting a VTD vehicle must be the x-position given as a double.", outputStorage);
            }
            // y
            if (!server.readTypeCheckingDouble(inputStorage, y)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The fourth parameter for setting a VTD vehicle must be the y-position given as a double.", outputStorage);
            }
            // process
            if (!v->isOnRoad()) {
                break;
            }
            std::string origID = edgeID + " " + toString(laneNum);
            if (laneNum < 0) {
                edgeID = '-' + edgeID;
                laneNum = -laneNum;
            }
            Position pos(x, y);

            Position vehPos = v->getPosition();
            v->getBestLanes();
            bool report = server.vtdDebug();
            if (report) {
                std::cout << std::endl << "begin vehicle " << v->getID() << " vehPos:" << vehPos << " lane:" << v->getLane()->getID() << std::endl;
            }
            if (report) {
                std::cout << " want pos:" << pos << " edge:" << edgeID << " laneNum:" << laneNum << std::endl;
            }

            MSEdgeVector edgesA, edgesB, edgesC;
            MSLane* laneA, *laneB, *laneC;
            laneA = laneB = laneC = 0;
            SUMOReal lanePosA, lanePosB, lanePosC;
            SUMOReal bestDistanceA, bestDistanceB, bestDistanceC;
            bestDistanceA = bestDistanceB = bestDistanceC = 1000.;//pos.distanceSquaredTo2D(vehPos);
            int routeOffsetA, routeOffsetB, routeOffsetC;
            routeOffsetA = routeOffsetB = routeOffsetC = 0;
            // case a): edge/lane is known and matches route
            bool aFound = vtdMap_matchingEdgeLane(pos, origID, *v, server.vtdDebug(), bestDistanceA, &laneA, lanePosA, routeOffsetA, edgesA);
            // case b): position is at route, should be somewhere near to it
            bool bFound = vtdMap_matchingRoutePosition(pos, origID, *v, server.vtdDebug(), bestDistanceB, &laneB, lanePosB, routeOffsetB, edgesB);
            // case c) nearest matching lane
            bool cFound = vtdMap_matchingNearest(pos, origID, *v, server, server.vtdDebug(), bestDistanceC, &laneC, lanePosC, routeOffsetC, edgesC);
            //
            SUMOReal maxRouteDistance = 50;
            if (cFound && (bestDistanceA > maxRouteDistance && bestDistanceC > maxRouteDistance)) {
                // both route-based approach yield in a position too far away from the submitted --> new route!?
                server.setVTDControlled(v, laneC, lanePosC, routeOffsetC, edgesC);
            } else {
                // use the best we have
                if (bFound) {
                    server.setVTDControlled(v, laneB, lanePosB, routeOffsetB, edgesB);
                } else if (aFound) {
                    server.setVTDControlled(v, laneA, lanePosA, routeOffsetA, edgesA);
                } else if (cFound) {
                    server.setVTDControlled(v, laneC, lanePosC, routeOffsetC, edgesC);
                } else {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Could not map vehicle.", outputStorage);
                }
            }
        }
        break;
    case VAR_SET_LEADER_SPEED_AND_ACCELERATION:
    {
        SUMOReal leaderSpeed;
        SUMOReal leaderAcc;
        SUMOReal x,y;
        Position leaderPosition;
        SUMOReal leaderTime;

        leaderSpeed = inputStorage.readDouble();
        leaderAcc = inputStorage.readDouble();
        x=inputStorage.readDouble();
        y=inputStorage.readDouble();
        leaderPosition.set(x, y);
        leaderTime = inputStorage.readDouble();

        const MSCFModel_CC * model;
        model = dynamic_cast<const MSCFModel_CC*>(&v->getVehicleType().getCarFollowModel());
        assert(model);

        model->setLeaderInformation((MSVehicle *)v, leaderSpeed, leaderAcc, leaderPosition, leaderTime);
    }
    break;
    case VAR_SET_PRECEDING_SPEED_AND_ACCELERATION:
    {
        SUMOReal precSpeed;
        SUMOReal precAcc;
        SUMOReal x,y;
        Position precPosition;
        SUMOReal precTime;

        precSpeed = inputStorage.readDouble();
        precAcc = inputStorage.readDouble();
        x=inputStorage.readDouble();
        y=inputStorage.readDouble();
        precPosition.set(x, y);
        precTime = inputStorage.readDouble();

        const MSCFModel_CC * model;
        model = dynamic_cast<const MSCFModel_CC *>(&v->getVehicleType().getCarFollowModel());
        assert(model);

        model->setPrecedingInformation((MSVehicle *)v, precSpeed, precAcc, precPosition, precTime);
    }
    break;
    case VAR_SET_ACTIVE_CONTROLLER: {

        const MSCFModel_CC * model;
        model = dynamic_cast<const MSCFModel_CC*>(&v->getVehicleType().getCarFollowModel());
        assert(model);

        int activeController = inputStorage.readInt();

        model->setActiveController((const MSVehicle *)v, (enum Plexe::ACTIVE_CONTROLLER)activeController);

    }
    break;
    case VAR_SET_CC_DESIRED_SPEED: {

        const MSCFModel_CC * model;
        model = dynamic_cast<const MSCFModel_CC*>(&v->getVehicleType().getCarFollowModel());
        assert(model);

        double ccDesiredSpeed = inputStorage.readDouble();

        model->setCCDesiredSpeed((MSVehicle *)v, ccDesiredSpeed);


    }
    break;
    case VAR_SET_LANE_CHANGE_ACTION: {

        const MSCFModel_CC * model;
        model = dynamic_cast<const MSCFModel_CC*>(&v->getVehicleType().getCarFollowModel());
        assert(model);

        int action = inputStorage.readInt();
        assert(action >= Plexe::DRIVER_CHOICE && action <= Plexe::STAY_IN_CURRENT_LANE);

        model->setLaneChangeAction((MSVehicle *)v, (enum Plexe::PLATOONING_LANE_CHANGE_ACTION) action);

    }
    break;
    case VAR_SET_CONTROLLER_FAKE_DATA: {

        const MSCFModel_CC * model;
        model = dynamic_cast<const MSCFModel_CC*>(&v->getVehicleType().getCarFollowModel());
        assert(model);

        //TODO: only basic idea for data needed for joining. might need to be extended
        /* we expect to get from the upper application the data that the CACC needs, i.e.:
         * - front distance, front speed and front vehicle acceleration: this information
         *   regards the car that the vehicle joining the platoon will have directly in
         *   front. this data might be real or might be fake: for example, if the platoon
         *   management algorithm decides to set the vehicle as the new leader, there won't
         *   be a car in front, and the fake data will be used only for positioning. in the
         *   case of fake data, acceleration must be set to 0
         * - leader front speed and acceleration: this information is the same as previously
         *   described for vehicle in front, but regards the leader. again, if the vehicle
         *   is being set as the new leader, this data might be fake data
         */
        double frontDistance, frontSpeed, frontAcceleration;
        double leaderSpeed, leaderAcceleration;

        frontDistance = inputStorage.readDouble();
        frontSpeed = inputStorage.readDouble();
        frontAcceleration = inputStorage.readDouble();
        leaderSpeed = inputStorage.readDouble();
        leaderAcceleration = inputStorage.readDouble();

        model->setControllerFakeData(v, frontDistance, frontSpeed, frontAcceleration, leaderSpeed, leaderAcceleration);

    }
    break;
    case VAR_SET_FIXED_LANE: {

        const MSCFModel_CC * model;
        model = dynamic_cast<const MSCFModel_CC*>(&v->getVehicleType().getCarFollowModel());
        assert(model);

        int lane = inputStorage.readInt();
        model->setFixedLane((const MSVehicle *)v, lane);

    }
    break;
    case VAR_SET_ACC_HEADWAY_TIME: {

        const MSCFModel_CC * model;
        model = dynamic_cast<const MSCFModel_CC*>(&v->getVehicleType().getCarFollowModel());
        assert(model);

        double headway = inputStorage.readDouble();
        model->setACCHeadwayTime((const MSVehicle *)v, headway);

    }
    break;
    case VAR_SET_FIXED_ACCELERATION: {

        const MSCFModel_CC * model;
        model = dynamic_cast<const MSCFModel_CC*>(&v->getVehicleType().getCarFollowModel());
        assert(model);

        int activate = inputStorage.readInt();
        double acceleration = inputStorage.readDouble();
        model->setFixedAcceleration((const MSVehicle *)v, activate, acceleration);
        break;

    }
    case VAR_SET_CACC_SPACING: {

        const MSCFModel_CC * model;
        model = dynamic_cast<const MSCFModel_CC*>(&v->getVehicleType().getCarFollowModel());
        assert(model);

        double spacing = inputStorage.readDouble();
        model->setCACCConstantSpacing((const MSVehicle *)v, spacing);
        break;
    }
    case VAR_SET_GENERIC_INFORMATION: {

        const MSCFModel_CC * model;
        model = dynamic_cast<const MSCFModel_CC*>(&v->getVehicleType().getCarFollowModel());
        assert(model);

        //header of the message
        struct Plexe::CCDataHeader header;
        //content of the message
        void *content;

        //copy the header
        inputStorage.readBuffer((unsigned char *)&header, sizeof(struct Plexe::CCDataHeader));
        //now copy the actual content
        content = malloc(header.size);
        inputStorage.readBuffer((unsigned char *)content, header.size);
        //pass data to model
        model->setGenericInformation((const MSVehicle *)v, header, content);
        free(content);

        break;

    }
        default:
            try {
                if (!TraCIServerAPI_VehicleType::setVariable(CMD_SET_VEHICLE_VARIABLE, variable, getSingularType(v), server, inputStorage, outputStorage)) {
                    return false;
                }
            } catch (ProcessError& e) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, e.what(), outputStorage);
            }
            break;
    }
    server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_OK, warning, outputStorage);
    return true;
}


bool
TraCIServerAPI_Vehicle::vtdMap_matchingEdgeLane(const Position& pos, const std::string& origID, MSVehicle& v, bool report,
        SUMOReal& bestDistance, MSLane** lane, SUMOReal& lanePos, int& routeOffset, MSEdgeVector& edges) {
    const std::map<std::string, std::vector<MSLane*> >&  vtdMap = getOrBuildVTDMap();
    if (vtdMap.find(origID) == vtdMap.end()) {
        if (report) {
            std::cout << "  a failed - lane not in map" << std::endl;
        }
        return false;
    }
    const std::vector<MSLane*>& lanes = vtdMap.find(origID)->second;
    for (std::vector<MSLane*>::const_iterator i = lanes.begin(); i != lanes.end() && bestDistance > POSITION_EPS; ++i) {
        MSLane* l = *i;
        SUMOReal dist = l->getShape().distance(pos);
        if (report) {
            std::cout << "   a at lane " << l->getID() << " dist:" << dist << " best:" << bestDistance << std::endl;
        }
        if (dist < bestDistance) {
            bestDistance = dist;
            *lane = l;
        }
    }
    MSLane* pni = *lane;
    while (pni != 0 && pni->getEdge().getPurpose() == MSEdge::EDGEFUNCTION_INTERNAL && pni->getIncomingLanes().size() != 0) {
        pni = pni->getIncomingLanes()[0].lane;
    }
    if (pni == 0 || pni->getEdge().getPurpose() == MSEdge::EDGEFUNCTION_INTERNAL) {
        // not found
        if (report) {
            std::cout << "  a failed - no incoming lane" << std::endl;
        }
        return false;
    }
    const MSEdgeVector& tedges = v.getRoute().getEdges();
    MSEdgeVector::const_iterator p = std::find(tedges.begin() + v.getRoutePosition(), tedges.end(), &pni->getEdge());
    if (p != tedges.end()) {
        lanePos = MAX2(SUMOReal(0), MIN2(SUMOReal((*lane)->getLength() - POSITION_EPS), (*lane)->getShape().nearest_offset_to_point2D(pos, false)));
        routeOffset = std::distance(tedges.begin(), p) - v.getRoutePosition();
        if (report) {
            std::cout << "  a ok lane:" << (*lane)->getID() << " lanePos:" << lanePos << " routeOffset:" << routeOffset << std::endl;
        }
        return true;
    }
    if (report) {
        std::cout << "  a failed - route position beyond route length" << std::endl;
    }
    return false;
}


bool
TraCIServerAPI_Vehicle::vtdMap_matchingRoutePosition(const Position& pos, const std::string& origID, MSVehicle& v, bool report,
        SUMOReal& bestDistance, MSLane** lane, SUMOReal& lanePos, int& routeOffset, MSEdgeVector& edges) {

    int lastBestRouteEdge = 0;
    int lastRouteEdge = 0;
    MSLane* bestRouteLane = 0;
    const std::vector<MSLane*>& bestLaneConts = v.getBestLanesContinuation(v.getLane());
    for (std::vector<MSLane*>::const_iterator i = bestLaneConts.begin(); i != bestLaneConts.end() && bestDistance > POSITION_EPS; ++i) {
        MSEdge& e = (*i)->getEdge();
        if (i != bestLaneConts.begin() && e.getPurpose() != MSEdge::EDGEFUNCTION_INTERNAL) {
            ++lastRouteEdge;
        }
        const std::vector<MSLane*>& lanes = e.getLanes();
        for (std::vector<MSLane*>::const_iterator k = lanes.begin(); k != lanes.end() && bestDistance > POSITION_EPS; ++k) {
            MSLane* cl = *k;
            SUMOReal dist = cl->getShape().distance(pos);
            if (report) {
                std::cout << "   b at lane " << cl->getID() << " dist:" << dist << " best:" << bestDistance << std::endl;
            }
            if (dist < bestDistance) {
                bestDistance = dist;
                *lane = cl;
                lastBestRouteEdge = lastRouteEdge;
                if (e.getPurpose() == MSEdge::EDGEFUNCTION_INTERNAL) {
                    bestRouteLane = *i;
                } else {
                    bestRouteLane = *lane;
                }
            }
        }
    }
    if (bestRouteLane == 0) {
        if (report) {
            std::cout << "  b failed - no best route lane" << std::endl;
        }
        return false;
    }
    lanePos = MAX2(SUMOReal(0), MIN2(SUMOReal(bestRouteLane->getLength() - POSITION_EPS), bestRouteLane->getShape().nearest_offset_to_point2D(pos, false)));
    routeOffset = lastBestRouteEdge;
    if (report) {
        std::cout << "  b ok lane " << bestRouteLane->getID() << " lanePos:" << lanePos << " best:" << lastBestRouteEdge << std::endl;
    }
    return true;
}


bool
TraCIServerAPI_Vehicle::vtdMap_matchingNearest(const Position& pos, const std::string& origID, MSVehicle& v, traci::TraCIServer& server, bool report,
        SUMOReal& bestDistance, MSLane** lane, SUMOReal& lanePos, int& routeOffset, MSEdgeVector& edges) {
    unsigned int r = 0;
    SUMOReal minDist = 1 << (11);
    MSLane* minDistLane = 0;
    MSLane* nameMatchingLane = 0;
    SUMOReal minDistNameMatchingLane = 1 << (11);
    for (; minDistLane == 0 && r < 10 && nameMatchingLane == 0; ++r) {
        std::set<std::string> into;
        PositionVector shape;
        shape.push_back(pos);
        server.collectObjectsInRange(CMD_GET_EDGE_VARIABLE, shape, 1 << r, into);
        for (std::set<std::string>::const_iterator j = into.begin(); j != into.end(); ++j) {
            MSEdge* e = MSEdge::dictionary(*j);
            const std::vector<MSLane*>& lanes = e->getLanes();
            for (std::vector<MSLane*>::const_iterator k = lanes.begin(); k != lanes.end(); ++k) {
                MSLane* lane = *k;
                SUMOReal dist = lane->getShape().distance(pos);
                if (lane->knowsParameter("origId")) {
                    if (lane->getParameter("origId", "") == origID) {
                        if (dist < minDistNameMatchingLane) {
                            minDistNameMatchingLane = dist;
                            nameMatchingLane = lane;
                        }
                    }
                }
                if (dist < minDist) {
                    minDist = dist;
                    minDistLane = lane;
                }
            }
        }
    }
    *lane = nameMatchingLane != 0 ? nameMatchingLane : minDistLane;
    if (lane == 0) {
        if (report) {
            std::cout << "  c failed - no matching lane" << std::endl;
        }
        return false;
    }
    lanePos = (*lane)->interpolateGeometryPosToLanePos((*lane)->getShape().nearest_offset_to_point2D(pos, false));
    if (*lane == v.getLane()) {
        routeOffset = 0;
        if (report) {
            std::cout << "  c ok, on same lane" << std::endl;
        }
        return true;
    }
    MSEdge& destinationEdge = (*lane)->getEdge();
    MSEdge* routePos = &destinationEdge;
    while (routePos->getPurpose() == MSEdge::EDGEFUNCTION_INTERNAL) {
        routePos = &routePos->getLanes()[0]->getLogicalPredecessorLane()->getEdge();
    }
    r = 0;
    const MSRoute& route = v.getRoute();
    unsigned int c = v.getRoutePosition();
    unsigned int l = (int)route.getEdges().size();
    unsigned int rindex = 0;
    bool found = false;
    while (!found && ((int)(c - r) >= 0 || c + r < l)) {
        if ((int)(c - r) >= 0 && route[c - r] == routePos) {
            rindex = c - r;
            found = true;
        }
        if (c + r < l && route[c + r] == routePos) {
            rindex = c + r;
            found = true;
        }
        ++r;
    }
    if (found) {
        // the matching lane is part of the route
        routeOffset = rindex - v.getRoutePosition();
        if (report) {
            std::cout << "  c ok, on a different edge of same route" << std::endl;
        }
        return true;
    }
    // build new route
    MSLane* firstLane = *lane;
    if (destinationEdge.getPurpose() != MSEdge::EDGEFUNCTION_INTERNAL) {
        edges.push_back(&destinationEdge);
    } else {
        firstLane = (*lane)->getLogicalPredecessorLane();
        edges.push_back(&firstLane->getEdge());
    }
    const MSLinkCont& lc = firstLane->getLinkCont();
    if (lc.size() != 0 && lc[0]->getLane() != 0) {
        edges.push_back(&lc[0]->getLane()->getEdge());
    }
    if (report) {
        std::cout << "  c ok, on a different route" << std::endl;
    }
    return true;
}


bool
TraCIServerAPI_Vehicle::commandDistanceRequest(traci::TraCIServer& server, tcpip::Storage& inputStorage,
        tcpip::Storage& outputStorage, const MSVehicle* v) {
    if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
        return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Retrieval of distance requires a compound object.", outputStorage);
    }
    if (inputStorage.readInt() != 2) {
        return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Retrieval of distance requires position and distance type as parameter.", outputStorage);
    }

    Position pos;
    std::pair<const MSLane*, SUMOReal> roadPos;

    // read position
    int posType = inputStorage.readUnsignedByte();
    switch (posType) {
        case POSITION_ROADMAP:
            try {
                std::string roadID = inputStorage.readString();
                roadPos.second = inputStorage.readDouble();
                roadPos.first = TraCIServerAPI_Simulation::getLaneChecking(roadID, inputStorage.readUnsignedByte(), roadPos.second);
                pos = roadPos.first->getShape().positionAtOffset(roadPos.second);
            } catch (TraCIException& e) {
                return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, e.what(), outputStorage);
            }
            break;
        case POSITION_2D:
        case POSITION_3D: {
            const double p1x = inputStorage.readDouble();
            const double p1y = inputStorage.readDouble();
            pos.set(p1x, p1y);
        }
        if (posType == POSITION_3D) {
            inputStorage.readDouble();		// z value is ignored
        }
        roadPos = TraCIServerAPI_Simulation::convertCartesianToRoadMap(pos);
        break;
        default:
            return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Unknown position format used for distance request", outputStorage);
    }

    // read distance type
    int distType = inputStorage.readUnsignedByte();

    SUMOReal distance = INVALID_DOUBLE_VALUE;
    if (v->isOnRoad()) {
        if (distType == REQUEST_DRIVINGDIST) {
            distance = v->getRoute().getDistanceBetween(v->getPositionOnLane(), roadPos.second,
                       v->getEdge(), &roadPos.first->getEdge());
            if (distance == std::numeric_limits<SUMOReal>::max()) {
                distance = INVALID_DOUBLE_VALUE;
            }
        } else {
            // compute air distance (default)
            distance = v->getPosition().distanceTo(pos);
        }
    }
    // write response command
    outputStorage.writeUnsignedByte(TYPE_DOUBLE);
    outputStorage.writeDouble(distance);
    return true;
}


// ------ helper functions ------
bool
TraCIServerAPI_Vehicle::getPosition(const std::string& id, Position& p) {
    MSVehicle* v = dynamic_cast<MSVehicle*>(MSNet::getInstance()->getVehicleControl().getVehicle(id));
    if (v == 0) {
        return false;
    }
    p = v->getPosition();
    return true;
}


MSVehicleType&
TraCIServerAPI_Vehicle::getSingularType(SUMOVehicle* const veh) {
    const MSVehicleType& oType = veh->getVehicleType();
    std::string newID = oType.getID().find('@') == std::string::npos ? oType.getID() + "@" + veh->getID() : oType.getID();
    MSVehicleType* type = MSVehicleType::build(newID, &oType);
    static_cast<MSVehicle*>(veh)->replaceVehicleType(type);
    return *type;
}


#include <microsim/MSEdgeControl.h>

const std::map<std::string, std::vector<MSLane*> >&
TraCIServerAPI_Vehicle::getOrBuildVTDMap() {
    if (gVTDMap.size() == 0) {
        const std::vector<MSEdge*>& edges = MSNet::getInstance()->getEdgeControl().getEdges();
        for (std::vector<MSEdge*>::const_iterator i = edges.begin(); i != edges.end(); ++i) {
            const std::vector<MSLane*>& lanes = (*i)->getLanes();
            for (std::vector<MSLane*>::const_iterator j = lanes.begin(); j != lanes.end(); ++j) {
                if ((*j)->knowsParameter("origId")) {
                    std::string origID = (*j)->getParameter("origId", "");
                    if (gVTDMap.find(origID) == gVTDMap.end()) {
                        gVTDMap[origID] = std::vector<MSLane*>();
                    }
                    gVTDMap[origID].push_back(*j);
                }
            }
        }
        if (gVTDMap.size() == 0) {
            gVTDMap["unknown"] = std::vector<MSLane*>();
        }
    }
    return gVTDMap;
}


#endif


/****************************************************************************/

