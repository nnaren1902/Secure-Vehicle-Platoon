/****************************************************************************/
/// @file    MSRoute.cpp
/// @author  Daniel Krajzewicz
/// @author  Friedemann Wesner
/// @author  Michael Behrisch
/// @date    Sept 2002
/// @version $Id$
///
// A vehicle route
/****************************************************************************/
// SUMO, Simulation of Urban MObility; see http://sumo.sourceforge.net/
// Copyright (C) 2001-2013 DLR (http://www.dlr.de/) and contributors
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

#include <cassert>
#include <algorithm>
#include <limits>
#include "MSRoute.h"
#include "MSEdge.h"
#include "MSLane.h"
#include <utils/common/FileHelpers.h>
#include <utils/common/RGBColor.h>
#include <utils/iodevices/BinaryInputDevice.h>
#include <utils/iodevices/OutputDevice.h>

#ifdef CHECK_MEMORY_LEAKS
#include <foreign/nvwa/debug_new.h>
#endif // CHECK_MEMORY_LEAKS


// ===========================================================================
// static member variables
// ===========================================================================
MSRoute::RouteDict MSRoute::myDict;
MSRoute::RouteDistDict MSRoute::myDistDict;
unsigned int MSRoute::MaxRouteDistSize = std::numeric_limits<unsigned int>::max();


// ===========================================================================
// member method definitions
// ===========================================================================
MSRoute::MSRoute(const std::string& id,
                 const MSEdgeVector& edges,
                 unsigned int references, const RGBColor* const c,
                 const std::vector<SUMOVehicleParameter::Stop>& stops)
    : Named(id), myEdges(edges),
      myReferenceCounter(references),
      myColor(c), myStops(stops) {}


MSRoute::~MSRoute() {
    delete myColor;
}


MSRouteIterator
MSRoute::begin() const {
    return myEdges.begin();
}


MSRouteIterator
MSRoute::end() const {
    return myEdges.end();
}


unsigned
MSRoute::size() const {
    return (unsigned) myEdges.size();
}


const MSEdge*
MSRoute::getLastEdge() const {
    assert(myEdges.size() > 0);
    return myEdges[myEdges.size() - 1];
}


void
MSRoute::addReference() const {
    myReferenceCounter++;
}


void
MSRoute::release() const {
    myReferenceCounter--;
    if (myReferenceCounter == 0) {
        myDict.erase(myID);
        delete this;
    }
}


bool
MSRoute::dictionary(const std::string& id, const MSRoute* route) {
    if (myDict.find(id) == myDict.end() && myDistDict.find(id) == myDistDict.end()) {
        myDict[id] = route;
        return true;
    }
    return false;
}


bool
MSRoute::dictionary(const std::string& id, RandomDistributor<const MSRoute*>* routeDist) {
    if (myDict.find(id) == myDict.end() && myDistDict.find(id) == myDistDict.end()) {
        myDistDict[id] = routeDist;
        return true;
    }
    return false;
}


const MSRoute*
MSRoute::dictionary(const std::string& id) {
    RouteDict::iterator it = myDict.find(id);
    if (it == myDict.end()) {
        RouteDistDict::iterator it2 = myDistDict.find(id);
        if (it2 == myDistDict.end() || it2->second->getOverallProb() == 0) {
            return 0;
        }
        return it2->second->get();
    }
    return it->second;
}


RandomDistributor<const MSRoute*>*
MSRoute::distDictionary(const std::string& id) {
    RouteDistDict::iterator it2 = myDistDict.find(id);
    if (it2 == myDistDict.end()) {
        return 0;
    }
    return it2->second;
}


void
MSRoute::clear() {
    for (RouteDistDict::iterator i = myDistDict.begin(); i != myDistDict.end(); ++i) {
        delete i->second;
    }
    myDistDict.clear();
    for (RouteDict::iterator i = myDict.begin(); i != myDict.end(); ++i) {
        delete i->second;
    }
    myDict.clear();
}


void
MSRoute::insertIDs(std::vector<std::string>& into) {
    into.reserve(myDict.size() + myDistDict.size() + into.size());
    for (RouteDict::const_iterator i = myDict.begin(); i != myDict.end(); ++i) {
        into.push_back((*i).first);
    }
    for (RouteDistDict::const_iterator i = myDistDict.begin(); i != myDistDict.end(); ++i) {
        into.push_back((*i).first);
    }
}


int
MSRoute::writeEdgeIDs(OutputDevice& os, const MSEdge* const from, const MSEdge* const upTo) const {
    int numWritten = 0;
    MSEdgeVector::const_iterator i = myEdges.begin();
    if (from != 0) {
        i = std::find(myEdges.begin(), myEdges.end(), from);
    }
    for (; i != myEdges.end(); ++i) {
        if ((*i) == upTo) {
            return numWritten;
        }
        os << (*i)->getID();
        numWritten++;
        if (upTo || i != myEdges.end() - 1) {
            os << ' ';
        }
    }
    return numWritten;
}


bool
MSRoute::containsAnyOf(const std::vector<MSEdge*>& edgelist) const {
    std::vector<MSEdge*>::const_iterator i = edgelist.begin();
    for (; i != edgelist.end(); ++i) {
        if (contains(*i)) {
            return true;
        }
    }
    return false;
}


const MSEdge*
MSRoute::operator[](unsigned index) const {
    return myEdges[index];
}


#ifdef HAVE_INTERNAL
void
MSRoute::dict_saveState(OutputDevice& out) {
    for (RouteDict::iterator it = myDict.begin(); it != myDict.end(); ++it) {
        out.openTag(SUMO_TAG_ROUTE).writeAttr(SUMO_ATTR_ID, (*it).second->getID());
        out.writeAttr(SUMO_ATTR_STATE, (*it).second->myReferenceCounter);
        out.writeAttr(SUMO_ATTR_EDGES, (*it).second->myEdges).closeTag();
    }
    for (RouteDistDict::iterator it = myDistDict.begin(); it != myDistDict.end(); ++it) {
        out.openTag(SUMO_TAG_ROUTE_DISTRIBUTION).writeAttr(SUMO_ATTR_ID, (*it).first);
        out.writeAttr(SUMO_ATTR_ROUTES, (*it).second->getVals());
        out.writeAttr(SUMO_ATTR_PROBS, (*it).second->getProbs());
        out.closeTag();
    }
}
#endif


SUMOReal
MSRoute::getLength() const {
    SUMOReal ret = 0;
    for (MSEdgeVector::const_iterator i = myEdges.begin(); i != myEdges.end(); ++i) {
        ret += (*i)->getLength();
    }
    return ret;
}


SUMOReal
MSRoute::getDistanceBetween(SUMOReal fromPos, SUMOReal toPos, const MSEdge* fromEdge, const MSEdge* toEdge) const {
    bool isFirstIteration = true;
    SUMOReal distance = -fromPos;
    MSEdgeVector::const_iterator it = std::find(myEdges.begin(), myEdges.end(), fromEdge);

    if (it == myEdges.end() || std::find(it, myEdges.end(), toEdge) == myEdges.end()) {
        // start or destination not contained in route
        return std::numeric_limits<SUMOReal>::max();
    }
    if (fromEdge == toEdge && fromPos <= toPos) {
        // destination position is on start edge
        return (toPos - fromPos);
    }
    for (; it != end(); ++it) {
        if ((*it) == toEdge && !isFirstIteration) {
            distance += toPos;
            break;
        } else {
            const std::vector<MSLane*>& lanes = (*it)->getLanes();
            distance += lanes[0]->getLength();
#ifdef HAVE_INTERNAL_LANES
            // add length of internal lanes to the result
            for (std::vector<MSLane*>::const_iterator laneIt = lanes.begin(); laneIt != lanes.end(); ++laneIt) {
                const MSLinkCont& links = (*laneIt)->getLinkCont();
                for (MSLinkCont::const_iterator linkIt = links.begin(); linkIt != links.end(); ++linkIt) {
                    if ((*linkIt) == 0 || (*linkIt)->getLane() == 0) {
                        continue;
                    }
                    std::string succLaneId = (*(it + 1))->getLanes()[0]->getID();
                    if ((*linkIt)->getLane()->getID().compare(succLaneId) == 0) {
                        distance += (*linkIt)->getLength();
                    }
                }
            }
#endif
        }
        isFirstIteration = false;
    }
    return distance;
}


const RGBColor&
MSRoute::getColor() const {
    if (myColor == 0) {
        return RGBColor::DEFAULT_COLOR;
    }
    return *myColor;
}


const std::vector<SUMOVehicleParameter::Stop>&
MSRoute::getStops() const {
    return myStops;
}


/****************************************************************************/

