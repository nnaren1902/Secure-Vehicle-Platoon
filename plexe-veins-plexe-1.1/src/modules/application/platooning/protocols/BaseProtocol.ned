//
// Copyright (C) 2012-2014 Michele Segata <segata@ccs-labs.org>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

package org.mixim.modules.application.platooning.protocols;

import org.mixim.base.modules.IBaseApplLayer;

//
// Define the base protocol for platooning. every improved
// protocol must derive from this
//
moduleinterface BaseProtocol extends IBaseApplLayer
{

    parameters:
        volatile double beaconingInterval @unit(seconds);// = default(0.1 seconds);
        //priority (AC) for the messages. 0 = AC_BK, 3 = AC_VO
        int priority;// = default(0);
        //size of platooning messages
        int packetSize;// = default(25);
        //for how much time should the simulation go on after starting the communication?
        int communicationDuration @unit("s");
        //send actual or controller-computed acceleration
        bool useControllerAcceleration;
		@display("i=block/network2");
        @class(BaseProtocol);
     gates:
         input upperLayerIn;
         output upperLayerOut;
         input upperControlIn;
         output upperControlOut;
}
