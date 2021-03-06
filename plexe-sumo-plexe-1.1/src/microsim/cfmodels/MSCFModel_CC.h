/****************************************************************************/
/// @file    MSCFModel_CC.h
/// @author  Michele Segata
/// @date    Wed, 18 Apr 2012
/// @version $Id: $
///
// A series of automatic Cruise Controllers (CC, ACC, CACC)
/****************************************************************************/
// SUMO, Simulation of Urban MObility; see http://sumo.sourceforge.net/
// Copyright (C) 2001-2011 DLR (http://www.dlr.de/) and contributors
// Copyright (C) 2012-2014 Michele Segata (segata@ccs-labs.org)
/****************************************************************************/
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
/****************************************************************************/
#ifndef MSCFMODEL_CC_H
#define MSCFMODEL_CC_H

// ===========================================================================
// included modules
// ===========================================================================
#ifdef _MSC_VER
#include <windows_config.h>
#else
#include <config.h>
#endif

#include "CC_Const.h"
#include <microsim/MSCFModel.h>
#include <microsim/MSLane.h>
#include <microsim/MSVehicle.h>
#include <microsim/MSVehicleType.h>
#include <utils/xml/SUMOXMLDefinitions.h>
#include <microsim/cfmodels/MSCFModel_Krauss.h>


// ===========================================================================
// class definitions
// ===========================================================================
/** @class MSCFModel_CC
 * @brief A set of automatic Cruise Controllers, including classic Cruise
 * Control (CC), Adaptive Cruise Control (ACC) and Cooperative Adaptive Cruise
 * Control (CACC). Take as references the chapters 5, 6 and 7 of the Rajamani's
 * book "Vehicle dynamics and control" (2011).
 * This model is meant to be used for simulation of platooning systems in mixed
 * scenarios, so with automatic and driver controlled vehicles.
 * The platooning manager is a distributed application implemented for veins
 * (so for omnet++) supported by a 802.11p based communication protocol, which
 * will determine the actions to be performed (such as switching on the
 * automatic controller, or the lane to move to) and communicate them to this
 * car following models via TraCI
 * @see MSCFModel
 */
class MSCFModel_CC : public MSCFModel {
public:

    /**
     * @struct FAKE_CONTROLLER_DATA
     * @brief represent the set of fake data which is sent to the controller in
     * order to automatically make the car move to a precise position before
     * joining the platoon.
     * we expect to get from the upper application the data that the CACC needs, i.e.:
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
    struct FAKE_CONTROLLER_DATA {
        double frontDistance;
        double frontSpeed;
        double frontAcceleration;
        double leaderSpeed;
        double leaderAcceleration;
    };

    /** @brief Constructor
     * @param[in] accel The maximum acceleration that controllers can output (def. 1.5 m/s^2)
     * @param[in] decel The maximum deceleration that ACC and CACC controllers can output (def. 6 m/s^2)
     * @param[in] ccDecel The maximum deceleration that the CC can output (def. 1.5 m/s^2)
     * @param[in] headwayTime the headway gap for ACC (be aware of instabilities) (def. 1.5 s)
     * @param[in] constantSpacing the constant gap for CACC (def. 5 m)
     * @param[in] kp design constant for CC (def. 1)
     * @param[in] lambda design constant for ACC (def. 0.1)
     * @param[in] c1 design constant for CACC (def. 0.5)
     * @param[in] xi design constant for CACC (def. 1)
     * @param[in] omegaN design constant for CACC (def. 0.2)
     * @param[in] tau engine time constant used for actuation lag (def. 0.5 s)
     * @param[in] lanesCount number of lanes of the highway
     * @param[in] ccAccel the maximum acceleration the CC can apply
     */
    MSCFModel_CC(const MSVehicleType* vtype, SUMOReal accel, SUMOReal decel,
                 SUMOReal ccDecel, SUMOReal headwayTime, SUMOReal constantSpacing,
                 SUMOReal kp, SUMOReal lambda, SUMOReal c1, SUMOReal xi,
                 SUMOReal omegaN, SUMOReal tau, int lanesCount, SUMOReal ccAccel);

    /// @brief Destructor
    ~MSCFModel_CC();


    /// @name Implementations of the MSCFModel interface
    /// @{

    /** @brief Applies interaction with stops and lane changing model influences
     * @param[in] veh The ego vehicle
     * @param[in] vPos The possible velocity
     * @return The velocity after applying interactions with stops and lane change model influences
     */
    SUMOReal moveHelper(MSVehicle* const veh, SUMOReal vPos) const;


    /** @brief Computes the vehicle's safe speed (no dawdling)
     * @param[in] veh The vehicle (EGO)
     * @param[in] speed The vehicle's speed
     * @param[in] gap2pred The (netto) distance to the LEADER
     * @param[in] predSpeed The speed of LEADER
     * @return EGO's safe speed
     * @see MSCFModel::ffeV
     */
    SUMOReal followSpeed(const MSVehicle* const veh, SUMOReal speed, SUMOReal gap2pred, SUMOReal predSpeed, SUMOReal predMaxDecel) const;


    /** @brief Computes the vehicle's safe speed for approaching a non-moving obstacle (no dawdling)
     * @param[in] veh The vehicle (EGO)
     * @param[in] gap2pred The (netto) distance to the the obstacle
     * @return EGO's safe speed for approaching a non-moving obstacle
     * @see MSCFModel::ffeS
     * @todo generic Interface, models can call for the values they need
     */
    SUMOReal stopSpeed(const MSVehicle* const veh, SUMOReal gap2pred) const;

    /** @brief Computes the vehicle's safe speed (no dawdling)
     *
     * Returns the velocity of the vehicle in dependence to the vehicle's and its leader's values and the distance between them.
     * @param[in] veh The vehicle (EGO)
     * @param[in] speed The vehicle's speed
     * @param[in] seen The look ahead distance
     * @param[in] maxSpeed The maximum allowed speed
     * @return EGO's safe speed
     */
    SUMOReal freeSpeed(const MSVehicle* const veh, SUMOReal speed, SUMOReal seen, SUMOReal maxSpeed) const;

    SUMOReal maxNextSpeed(SUMOReal speed) const;


    /** @brief Returns the maximum gap at which an interaction between both vehicles occurs
     *
     * "interaction" means that the LEADER influences EGO's speed.
     * @param[in] veh The EGO vehicle
     * @param[in] vL LEADER's speed
     * @return The interaction gap
     * @todo evaluate signature
     * @see MSCFModel::interactionGap
     */
    SUMOReal interactionGap(const MSVehicle* const , SUMOReal vL) const;


    /** @brief Returns the model's name
     * @return The model's name
     * @see MSCFModel::getModelName
     */
    int getModelID() const {
        return SUMO_TAG_CF_CC;
    }
    /// @}



    /** @brief Duplicates the car-following model
     * @param[in] vtype The vehicle type this model belongs to (1:1)
     * @return A duplicate of this car-following model
     */
    MSCFModel* duplicate(const MSVehicleType* vtype) const;


    VehicleVariables* createVehicleVariables() const {
        MSCFModel_CC::VehicleVariables *vars = new MSCFModel_CC::VehicleVariables();
        vars->caccSpacing = myConstantSpacing;
        vars->caccC1 = myC1;
        vars->caccXi = myXi;
        vars->caccOmegaN = myOmegaN;
        vars->engineTau = myTau;
        //we cannot invoke recomputeParameters() because we have no pointer to the MSVehicle class
        vars->caccAlpha1 = 1 - vars->caccC1;
        vars->caccAlpha2 = vars->caccC1;
        vars->caccAlpha3 = -(2 * vars->caccXi - vars->caccC1 * (vars->caccXi + sqrt(vars->caccXi * vars->caccXi - 1))) * vars->caccOmegaN;
        vars->caccAlpha4 = -(vars->caccXi + sqrt(vars->caccXi* vars->caccXi - 1)) * vars->caccOmegaN * vars->caccC1;
        vars->caccAlpha5 = -vars->caccOmegaN * vars->caccOmegaN;
        vars->engineAlpha = TS / (vars->engineTau + TS);
        vars->engineOneMinusAlpha = 1 - vars->engineAlpha;
        return (VehicleVariables *)vars;
    }

    /**
     * @brief set the cruise control desired speed. notice that this command does
     * not enable the cruise control and can also be used when cruise control is
     * already active
     *
     * @param[in] veh the vehicle for which the desired speed has to be changed
     * @param[in] ccDesiredSpeed the desired speed in m/s
     */
    void setCCDesiredSpeed(const MSVehicle* veh, SUMOReal ccDesiredSpeed) const;

    /**
     * @brief set CACC desired constant spacing
     *
     * @param[in] veh the vehicle to set constant spacing for
     * @param[in] spacing the spacing in meters
     */
    void setCACCConstantSpacing(const MSVehicle * veh, SUMOReal spacing) const;

    /**
     * @brief returns CACC desired constant spacing
     *
     * @param[in] veh the vehicle to get constant spacing of
     * @return spacing the spacing in meters
     */
    SUMOReal getCACCConstantSpacing(const MSVehicle * veh) const;

    /**
     * @brief set the information about the platoon leader. This method should be invoked
     * by TraCI when a wireless message with such data is received. For testing, it might
     * be also invoked from SUMO source code
     *
     * @param[in] veh the vehicle for which the data must be saved
     * @param[in] speed the leader speed
     * @param[in] acceleration the leader acceleration
     * @param[in] position the position of the leader
     * @param[in] time the time at which this data was read from leader's sensors
     */
    void setLeaderInformation(const MSVehicle* veh, SUMOReal speed, SUMOReal acceleration, Position position, SUMOReal time)  const;

    /**
     * @brief set the information about preceding vehicle. This method should be invoked
     * by TraCI when a wireless message with such data is received. For testing, it might
     * be also invoked from SUMO source code
     *
     * @param[in] veh the vehicle for which the data must be saved
     * @param[in] speed the speed of the preceding vehicle
     * @param[in] acceleration the acceleration of the preceding vehicle
     * @param[in] position the position of the preceding vehicle
     * @param[in] time the time at which this data was read from preceding vehicle's sensors
     *
     */
    void setPrecedingInformation(const MSVehicle* const veh, SUMOReal speed, SUMOReal acceleration, Position position, SUMOReal time) const;

    /**
     * @brief set the information about a generic car. This method should be invoked
     * by TraCI when a wireless message with such data is received. For testing, it might
     * be also invoked from SUMO source code
     *
     * @param[in] veh the vehicle for which the data must be saved
     * @param[in] speed the leader speed
     * @param[in] acceleration the leader acceleration
     * @param[in] position the position of the leader
     * @param[in] time the time at which this data was read from leader's sensors
     */
//    void setVehicleInformation(const MSVehicle* veh, SUMOReal speed, SUMOReal acceleration, Position position, SUMOReal time)  const;

    /**
     * @brief generic data passing method to this class
     *
     * @param[in] veh the vehicle to which data is directed to
     * @param[in] header header including information about the actual message
     * @param[in] content pointer to the actual content
     */
    void setGenericInformation(const MSVehicle* veh, const struct Plexe::CCDataHeader &header, const void *content) const;

    /**
     * @brief generic data retrieval method from this class
     *
     * @param[in] veh the vehicle from which data is requested
     * @param[in] request struct including request type, and request parameters length
     * @param[in] reqParams parameters connected to the request, if any
     * @param[out] content pointer where data will be copied to
     * @return actual size of data copied into content
     */
    int getGenericInformation(const MSVehicle *veh, struct Plexe::CCDataHeader request, const void *reqParams, void *content) const;

    /**
     * @brief get the information about a vehicle. This can be used by TraCI in order to
     * get speed and acceleration of the platoon leader before sending them to other
     * vehicles
     *
     * @param[in] veh the vehicle for which the data is requested
     * @param[out] speed where the speed is written
     * @param[out] acceleration where the acceleration is written
     * @param[out] controllerAcceleration the last acceleration value computed by
     * the controller will be written in this variable. This might be different from
     * acceleration because of actuation lag
     */
    void getVehicleInformation(const MSVehicle* veh, SUMOReal& speed, SUMOReal& acceleration, SUMOReal& controllerAcceleration, Position &position, SUMOReal &time) const;

    /**
     * @brief switch on the ACC, so disabling the human driver car control
     *
     * @param[in] veh the vehicle for which the ACC must be switched on
     * @param[in] ccDesiredSpeed the cruise control speed
     */
    void switchOnACC(const MSVehicle *veh, double ccDesiredSpeed) const;

    /**
     * @brief set the headway time for the ACC controller. This might be useful for
     * analyzing instabilities, for example by setting headway time to a value like
     * 0.3 seconds
     *
     * @param[in] veh the vehicle for which the ACC headway time must be changed
     * @param[in] headwayTime headway time to be set
     */
    void setACCHeadwayTime(const MSVehicle *veh, double headwayTime) const;

    /**
     * @brief enable/disable the use of a fixed acceleration, instead of using the one computed by
     * the controllers. the acceleration is used only if the CC/ACC/CACC is enabled
     *
     * @param[in] veh the vehicle for which the fixed acceleration has to be set
     * @param[in] activate activate (1) or deactivate (0) the fixed acceleration
     * @param[in] acceleration the fixed acceleration to be used (if activate == 1)
     */
    void setFixedAcceleration(const MSVehicle *veh, int activate, double acceleration) const;

    /**
     * @brief set the active controller. Notice that if the selected controller is ACC or CACC
     * the setCCDesiredSpeed must be invoked before, otherwise the speed is set to the default
     * value
     *
     * @param[in] veh the vehicle for which the action is requested
     * @param[in] activeController the controller to be set as active, which can be either the
     * driver, or the ACC or the CACC
     */
    void setActiveController(const MSVehicle *veh, enum Plexe::ACTIVE_CONTROLLER activeController)  const;

    /**
     * @brief return the currently active controller
     *
     * @param[in] veh the vehicle for which the action is requested
     * @return the currently active controller
     */
    enum Plexe::ACTIVE_CONTROLLER getActiveController(const MSVehicle *veh) const;

    /**
     * @brief gets the lane change action requested by the platooning management system.
     * The action is set by the platooning manager via TraCI and it is requested by the
     * MSCACCLaneChanger class
     *
     * @return the lane changing action to be performed
     */
    enum Plexe::PLATOONING_LANE_CHANGE_ACTION getLaneChangeAction(const MSVehicle *veh) const;

    /**
     * @brief sets the lane change action requested by the platooning management system.
     */
    void setLaneChangeAction(const MSVehicle *veh, enum Plexe::PLATOONING_LANE_CHANGE_ACTION action) const;

    /**
     * @brief sets the lane a car should stay in. might be useful to form platoons in any
     * lane and not in a dedicated one.
     * After setting the lane, the lane change action (enum PLATOONING_LANE_CHANE_ACTION) will
     * be set to STAY_IN_CURRENT_LANE. Afterwards, it might be possible to either call this
     * method again to change lane (and still keep it fixed) or setting the lane change action
     * to DRIVER_CHOICE, to let the driver model chose the appropriate lane
     *
     * @param[in] lane index of the lane to move to, starting from 0. 0 is the rightmost.
     * If there is no such lane, the request will be ignored
     */
    void setFixedLane(const MSVehicle *veh, int lane) const;

    /**
     * @brief return the data that is currently being measured by the radar
     */
    void getRadarMeasurements(const MSVehicle * veh, double &distance, double &relativeSpeed) const;

    /**
     * @brief set the fake data which the controller will use for joining while in the
     * management lane. Notice that the function can be used to set data about both leader and
     * front vehicle, or either one or the other. If the speed of one of the two is set to a
     * negative value, then the data about such vehicle is ignored
     */
    void setControllerFakeData(const MSVehicle *veh, double frontDistance, double frontSpeed, double frontAcceleration,
            double leaderSpeed, double leaderAcceleration) const;

    /**
     * @brief tells the module to ignore modifications to the state state of the vehicle. a class is going to invoke
     * methods like followSpeed, but not for changing actually moving the vehicle. for example the lane changer
     * invokes followSpeed to determine whether the car can gain some advantages by moving to another lane. this
     * advantage is computed by using the followSpeed method. in such case, the state of the vehicle should not
     * be changed
     */
    void setIgnoreModifications(const MSVehicle *veh, bool ignore) const;

    /**
     * @brief tells the module that in the last timestep the car has crashed (or not)
     *
     * @param[in] veh the vehicle
     * @param[in] crashed whether the car has crashed or not
     */
    void setCrashed(const MSVehicle *veh, bool crashed) const;

    /**
     * @brief tells whether the car has crashed or not
     *
     * @param[in] veh the vehicle
     * @return true if the vehicle has crashed, false otherwise
     */
    bool isCrashed(const MSVehicle *veh) const;

    /**
     * @brief returns the ACC computed acceleration when the faked
     * CACC is controlling the car. This can be used to check for
     * vehicles in front
     */
    double getACCAcceleration(const MSVehicle *veh) const;

    /**
     * @brief returns the number of lanes set in the configuration file
     */
    int getMyLanesCount() const;

private:

    /**
     * This enum tells to the _v method who has invoked it
     */
    enum CONTROLLER_INVOKER {
        STOP_SPEED,
        FOLLOW_SPEED,
        FREE_SPEED
    };

    /**
     * @brief Recomputes controller related parameters after setting them
     */
    void recomputeParameters(const MSVehicle *veh) const;

public:

    class VehicleVariables : public MSCFModel::VehicleVariables {
    public:
        VehicleVariables() : egoDataLastUpdate(0), egoSpeed(0), egoAcceleration(0), egoPreviousSpeed(0),
            frontDataLastUpdate(0), frontSpeed(0), radarFrontDistance(1000), frontAcceleration(0),
            radarFrontSpeed(0), leaderDataLastUpdate(0), leaderSpeed(0), leaderAcceleration(0),
            platoonId(""), isPlatoonLeader(false), ccDesiredSpeed(14), radarLastUpdate(0), activeController(Plexe::DRIVER),
            laneChangeAction(Plexe::DRIVER_CHOICE), followSpeedSetTime(0), controllerFollowSpeed(0), controllerFreeSpeed(0),
            ignoreModifications(false), fixedLane(-1), accHeadwayTime(1.5), useFixedAcceleration(0), fixedAcceleration(0),
            crashed(false), controllerAcceleration(0), followControllerAcceleration(0), freeControllerAcceleration(0),
            accAcceleration(0), followAccAcceleration(0), freeAccAcceleration(0), caccSpacing(5),
            leaderDataReadTime(0), frontDataReadTime(0), position(-1), nCars(8),
            caccXi(-1), caccOmegaN(-1), caccC1(-1), engineTau(-1), caccAlpha1(-1), caccAlpha2(-1),
            caccAlpha3(-1), caccAlpha4(-1), caccAlpha5(-1), engineAlpha(-1), engineOneMinusAlpha(-1) {
            fakeData.frontAcceleration = 0;
            fakeData.frontDistance = 0;
            fakeData.frontSpeed = 0;
            fakeData.leaderAcceleration = 0;
            fakeData.leaderSpeed = 0;
            leaderPosition.set(0, 0);
            frontPosition.set(0, 0);
        }

        /// @brief last time ego data has been updated
        SUMOTime egoDataLastUpdate;
        /// @brief current vehicle speed
        SUMOReal egoSpeed;
        /// @brief current vehicle acceleration
        SUMOReal egoAcceleration;
        /// @brief vehicle speed at previous timestep
        SUMOReal egoPreviousSpeed;
        /// @brief acceleration as computed by the controller, to be sent to other vehicles
        SUMOReal controllerAcceleration;
        /// @brief acceleration as computed by the controller when followSpeed is invoked
        SUMOReal followControllerAcceleration;
        /// @brief acceleration as computed by the controller when freeSpeed is invoked
        SUMOReal freeControllerAcceleration;

        /// @brief last time front vehicle data (speed and acceleration) has been updated
        SUMOTime frontDataLastUpdate;
        /// @brief current front vehicle speed
        SUMOReal frontSpeed;
        /// @brief current front vehicle acceleration (used by CACC)
        SUMOReal frontAcceleration;
        /// @brief current front vehicle position
        Position frontPosition;
        /// @brief when front vehicle data has been readed from GPS
        SUMOReal frontDataReadTime;
        /// @brief last timestep at which front vehicle data (distance) has been updated
        SUMOTime radarLastUpdate;
        /// @brief current front vehicle distance as provided by the radar
        SUMOReal radarFrontDistance;
        /// @brief current front vehicle speed as provided by the radar
        SUMOReal radarFrontSpeed;

        /// @brief headway time for ACC
        SUMOReal accHeadwayTime;
        /// @brief fixed spacing for CACC
        SUMOReal caccSpacing;

        /// @brief last time leader vehicle data has been updated
        SUMOTime leaderDataLastUpdate;
        /// @brief platoon's leader speed (used by CACC)
        SUMOReal leaderSpeed;
        /// @brief platoon's leader acceleration (used by CACC)
        SUMOReal leaderAcceleration;
        /// @brief platoon's leader position
        Position leaderPosition;
        /// @brief when leader data has been readed from GPS
        SUMOReal leaderDataReadTime;

        //time at which followSpeed has been invoked. In this way
        //we can tell moveHelper whether controllerFollowSpeed must
        //be used or not, by checking if this value has been set
        //in the current time step
        SUMOTime followSpeedSetTime;
        //speed computed by followSpeed
        double controllerFollowSpeed;
        //speed computed by freeSpeed
        double controllerFreeSpeed;

        //enable/disable the use of a constant, user defined acceleration instead of the one computed by the controller
        int useFixedAcceleration;
        //fixed acceleration to use
        double fixedAcceleration;

        /** tells the module to ignore modifications to the state state of the vehicle. a class is going to invoke
         *  methods like followSpeed, but not for changing actually moving the vehicle. for example the lane changer
         *  invokes followSpeed to determine whether the car can gain some advantages by moving to another lane. this
         *  advantage is computed by using the followSpeed method. in such case, the state of the vehicle should not
         *  be changed
         */
        bool ignoreModifications;

        //car collided in the last timestep
        bool crashed;

        /// @brief CC desired speed
        SUMOReal ccDesiredSpeed;
        /// @brief currently active controller
        enum Plexe::ACTIVE_CONTROLLER activeController;

        /// @brief fake controller data. @see FAKE_CONTROLLER_DATA
        struct FAKE_CONTROLLER_DATA fakeData;
        /** acceleration computed by the ACC system. This can be used during the approach phase to understand whether
         *  a slow vehicles is in front
         */
        double accAcceleration, followAccAcceleration, freeAccAcceleration;

        /// @brief lane change action to be performed as given by the platoon management application
        enum Plexe::PLATOONING_LANE_CHANGE_ACTION laneChangeAction;

        /// @brief fixed lane selected
        int fixedLane;

        //TODO: most probably the following variables needs to be moved to the application logic (i.e., network protocol)
        /// @brief own platoon id
        std::string platoonId;
        /// @brief is ego vehicle the leader?
        bool isPlatoonLeader;

        /// @brief data about vehicles in the platoon
        struct Plexe::VEHICLE_DATA vehicles[MAX_N_CARS];
        /// @brief my position within the platoon (0 = first car)
        int position;
        /// @brief number of cars in the platoon
        int nCars;

        /// @brief controller related parameters
        double caccXi;
        double caccOmegaN;
        double caccC1;
        double caccAlpha1, caccAlpha2, caccAlpha3, caccAlpha4, caccAlpha5;
        double engineTau, engineAlpha, engineOneMinusAlpha;
    };


private:
    SUMOReal _v(const MSVehicle* const veh, SUMOReal gap2pred, SUMOReal egoSpeed, SUMOReal predSpeed, SUMOReal desSpeed, enum CONTROLLER_INVOKER invoker) const;

    /** @brief controller for the CC which computes the acceleration to be applied. the value needs to be passed to the actuator
     *
     * @param[in] egoSpeed vehicle current speed
     * @param[in] desSpeed vehicle desired speed
     * @return the acceleration to be given to the actuator
     */
    SUMOReal _cc(const MSVehicle *veh, SUMOReal egoSpeed, SUMOReal desSpeed) const;

    /** @brief controller for the ACC which computes the acceleration to be applied. the value needs to be passed to the actuator
     *
     * @param[in] egoSpeed vehicle current speed
     * @param[in] desSpeed vehicle desired speed
     * @param[in] gap2pred the distance to preceding vehicle
     * @param[in] headwayTime the headway time ACC should maintain
     * @return the acceleration to be given to the actuator
     */
    SUMOReal _acc(const MSVehicle *veh, SUMOReal egoSpeed, SUMOReal predSpeed, SUMOReal gap2pred, SUMOReal headwayTime) const;

    /** @brief controller for the CACC which computes the acceleration to be applied. the value needs to be passed to the actuator
     *
     * @param[in] egoSpeed vehicle current speed
     * @param[in] desSpeed vehicle desired speed
     * @param[in] predAcceleration acceleration of preceding vehicle
     * @param[in] gap2pred the distance to preceding vehicle
     * @param[in] leaderSpeed the speed of the platoon leader
     * @param[in] leaderAcceleration the acceleration of the platoon leader
     * @param[in] spacing the spacing to be kept
     * @return the acceleration to be given to the actuator
     */
    SUMOReal _cacc(const MSVehicle *veh, SUMOReal egoSpeed, SUMOReal predSpeed, SUMOReal predAcceleration, SUMOReal gap2pred, SUMOReal leaderSpeed, SUMOReal leaderAcceleration, SUMOReal spacing) const;

    /** @brief computes the actual acceleration the actuator is able to apply to the car, given engine time constant and previous
     * acceleration
     *
     * @param[in] acceleration the acceleration to be applied, computed by the controller
     * @param[in] currentAcceleration the current car acceleration
     * @return the actual acceleration applied by the engine
     */
    SUMOReal _actuator(const MSVehicle *veh, SUMOReal acceleration, SUMOReal currentAcceleration) const;

    SUMOReal desiredSpeed(const MSVehicle* const veh) const {
        return MIN2(myType->getMaxSpeed(), veh->getLane()->getSpeedLimit());
    }


private:

    /// @brief the car following model which drives the car when automated cruising is disabled, i.e., the human driver
    MSCFModel *myHumanDriver;

    /// @brief The maximum deceleration that the CC can output
    const SUMOReal myCcDecel;

    /// @brief The maximum acceleration that the CC can output
    const SUMOReal myCcAccel;

    /// @brief the constant gap for CACC
    const SUMOReal myConstantSpacing;

    /// @brief design constant for CC
    const SUMOReal myKp;

    /// @brief design constant for ACC
    const SUMOReal myLambda;

    /// @brief design constant for CACC
    const SUMOReal myC1;

    /// @brief design constant for CACC
    const SUMOReal myXi;

    /// @brief design constant for CACC
    const SUMOReal myOmegaN;

    /// @brief engine time constant used for actuation lag
    const SUMOReal myTau;
    /// @brief the alpha parameter for the low-pass filter used to implement the actuation lag
    const SUMOReal myAlpha;
    /// @brief one minus alpha parameters
    const SUMOReal myOneMinusAlpha;

    /// @brief A computational shortcut for CACC
    const SUMOReal myAlpha1;
    /// @brief A computational shortcut for CACC
    const SUMOReal myAlpha2;
    /// @brief A computational shortcut for CACC
    const SUMOReal myAlpha3;
    /// @brief A computational shortcut for CACC
    const SUMOReal myAlpha4;
    /// @brief A computational shortcut for CACC
    const SUMOReal myAlpha5;

    /// @brief number of lanes in the highway, in the absence of on-/off-ramps. This is used
    /// to move to the correct lane even when a lane is added for on-/off-ramps
    const int myLanesCount;

};

#endif /* MSCFMODEL_CC_H */
