/****************************************************************************/
/// @file    MSCFModel.cpp
/// @author  Tobias Mayer
/// @author  Daniel Krajzewicz
/// @author  Jakob Erdmann
/// @author  Michael Behrisch
/// @author  Laura Bieker
/// @date    Mon, 27 Jul 2009
/// @version $Id$
///
// The car-following model abstraction
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

#include <math.h>
#include "MSCFModel.h"
#include "MSVehicleType.h"
#include "MSVehicle.h"
#include "MSLane.h"
#include "MSAbstractLaneChangeModel.h"


// ===========================================================================
// method definitions
// ===========================================================================
MSCFModel::MSCFModel(const MSVehicleType* vtype, const SUMOReal accel,
                     const SUMOReal decel, const SUMOReal headwayTime)
    : myType(vtype), myAccel(accel), myDecel(decel), myHeadwayTime(headwayTime) {
}


MSCFModel::~MSCFModel() {}


SUMOReal
MSCFModel::moveHelper(MSVehicle* const veh, SUMOReal vPos) const {
    const SUMOReal oldV = veh->getSpeed(); // save old v for optional acceleration computation
    const SUMOReal vSafe = MIN2(vPos, veh->processNextStop(vPos)); // process stops
    // we need the acceleration for emission computation;
    //  in this case, we neglect dawdling, nonetheless, using
    //  vSafe does not incorporate speed reduction due to interaction
    //  on lane changing
    const SUMOReal vMin = getSpeedAfterMaxDecel(oldV);
    const SUMOReal vMax = MIN3(veh->getLane()->getVehicleMaxSpeed(veh), maxNextSpeed(oldV, veh), vSafe);
    assert(vMin <= vMax);
    return veh->getLaneChangeModel().patchSpeed(vMin, vMax, vMax, *this);
}


SUMOReal
MSCFModel::interactionGap(const MSVehicle* const veh, SUMOReal vL) const {
    // Resolve the vsafe equation to gap. Assume predecessor has
    // speed != 0 and that vsafe will be the current speed plus acceleration,
    // i.e that with this gap there will be no interaction.
    const SUMOReal vNext = MIN2(maxNextSpeed(veh->getSpeed(), veh), veh->getLane()->getVehicleMaxSpeed(veh));
    const SUMOReal gap = (vNext - vL) *
                         ((veh->getSpeed() + vL) / (2.*myDecel) + myHeadwayTime) +
                         vL * myHeadwayTime;

    // Don't allow timeHeadWay < deltaT situations.
    return MAX2(gap, SPEED2DIST(vNext));
}


void
MSCFModel::leftVehicleVsafe(const MSVehicle* const ego, const MSVehicle* const neigh, SUMOReal& vSafe) const {
    if (neigh != 0 && neigh->getSpeed() > 60. / 3.6) {
        SUMOReal mgap = MAX2((SUMOReal) 0, neigh->getPositionOnLane() - neigh->getVehicleType().getLength() - ego->getPositionOnLane() - ego->getVehicleType().getMinGap());
        SUMOReal nVSafe = followSpeed(ego, ego->getSpeed(), mgap, neigh->getSpeed(), neigh->getCarFollowModel().getMaxDecel());
        if (mgap - neigh->getSpeed() >= 0) {
            vSafe = MIN2(vSafe, nVSafe);
        }
    }
}


SUMOReal
MSCFModel::maxNextSpeed(SUMOReal speed, const MSVehicle* const /*veh*/) const {
    return MIN2(speed + (SUMOReal) ACCEL2SPEED(getMaxAccel()), myType->getMaxSpeed());
}


SUMOReal
MSCFModel::brakeGap(SUMOReal speed) const {
    /* one possiblity to speed this up is to precalculate speedReduction * steps * (steps+1) / 2
       for small values of steps (up to 10 maybe) and store them in an array */
    const SUMOReal speedReduction = ACCEL2SPEED(getMaxDecel());
    const int steps = int(speed / speedReduction);
    return SPEED2DIST(steps * speed - speedReduction * steps * (steps + 1) / 2) + speed * myHeadwayTime;
}


SUMOReal
MSCFModel::freeSpeed(const MSVehicle* const veh, SUMOReal speed, SUMOReal seen, SUMOReal maxSpeed) const {
    UNUSED_PARAMETER(veh);
    // adapt speed to succeeding lane, no reaction time is involved
    // when breaking for y steps the following distance g is covered
    // (drive with v in the final step)
    // g = (y^2 + y) * 0.5 * b + y * v
    // y = ((((sqrt((b + 2.0*v)*(b + 2.0*v) + 8.0*b*g)) - b)*0.5 - v)/b)
    const SUMOReal b = ACCEL2SPEED(myDecel);
    const SUMOReal y = MAX2(0.0, ((sqrt((b + 2.0 * maxSpeed) * (b + 2.0 * maxSpeed) + 8.0 * b * seen) - b) * 0.5 - maxSpeed) / b);
    const SUMOReal yFull = floor(y);
    const SUMOReal exactGap = (yFull * yFull + yFull) * 0.5 * b + yFull * maxSpeed + (y > yFull ? maxSpeed : 0.0);
    return MAX2((SUMOReal)0.0, seen - exactGap) / (yFull + 1) + yFull * myDecel + maxSpeed;
}

/****************************************************************************/
