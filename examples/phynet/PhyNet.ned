//
// Copyright (C) 2013 OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
//

package inet.examples.phynet;


import inet.node.wsn.AccessPoint;
import inet.node.wsn.PhyNode;
import inet.physicallayer.contract.packetlevel.IRadioMedium;
import inet.visualizer.contract.IIntegratedVisualizer;
import inet.mobility.group.StorageHallCoordinator;
import inet.logger.Logger;


network PhyNet
{
    parameters:
        @display("bgu=m");
		int numNodes;
		string radioMediumType = default("IdealRadioMedium");
    submodules:
        visualizer: <default("IntegratedCanvasVisualizer")> like IIntegratedVisualizer if hasVisualizer() {
            parameters:
                @display("p=0.5,-0.8;is=s");
        }
        logger: Logger {
			parameters:
			  @display("p=3.5,-0.8;is=s");  
		}
        radioMedium: <radioMediumType> like IRadioMedium {
            parameters:
                @display("p=1.5,-0.8;is=s");
        }
        mobilityCoordinator: StorageHallCoordinator {
        	parameters:
        	    @display("p=2.5,-0.8;is=s");
        	    numItems = numNodes;          
        }
        accessPoint: AccessPoint {
            parameters:
                @display("p=1,1;i=,black,40");
        }
        node[numNodes]: PhyNode {
            parameters:
                @display("i=-;b=0.3,0.4,rect,,black,1");
        }
}

