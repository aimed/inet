//
// Copyright (C) 2013 OpenSim Ltd
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//
// author: Zoltan Bojthe
//


package inet.linklayer.wsn;

import inet.common.queue.IOutputQueue;
import inet.linklayer.contract.IWirelessNic;
import inet.linklayer.contract.IMACProtocol;
import inet.physicallayer.contract.packetlevel.IRadio;

// WirelessNic, that is capable to load different Mac-,Radio- und QueueModules.
//
//
module CsmaPhyNetNic like IWirelessNic
{
    parameters:
        @display("i=block/ifcard;bgb=214,335;bgl=53");
        string interfaceTableModule;
        string energySourceModule = default("");
        double bitrate @unit("bps");
        string macType = default("CSMA");
        string radioType = default("IdealRadio");
        *.interfaceTableModule = default(absPath(interfaceTableModule));
        *.energySourceModule = default(absPath(energySourceModule));
        **.bitrate = bitrate;
    gates:
        input upperLayerIn;
        output upperLayerOut;
        input radioIn @labels(IdealRadioFrame);
    submodules:
        mac: <macType> like IMACProtocol {
            parameters:
                @display("p=98,207");
        }
        radio: <radioType> like IRadio {
            parameters:
                @display("p=98,278");
        }
    connections:
        upperLayerIn --> { @display("m=n"); } --> mac.upperLayerIn;
        mac.lowerLayerOut --> radio.upperLayerIn;
        mac.upperLayerOut --> { @display("m=n"); } --> upperLayerOut;
        radioIn --> { @display("m=s"); } --> radio.radioIn;
        radio.upperLayerOut --> mac.lowerLayerIn;
}