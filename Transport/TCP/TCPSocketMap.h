//
// Copyright (C) 2004 Andras Varga
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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//


#ifndef __TCPSOCKETMAP_H
#define __TCPSOCKETMAP_H


#include <omnetpp.h>
#include <map>
#include "TCPSocket.h"


/**
 * Small utility class for managing a large number of TCPSocket objects.
 */
//TBD: need for lookup could be eliminated by adding a void *yourPtr into TCPConnection and TCPCommand
class TCPSocketMap
{
  protected:
    typedef std::map<int,TCPSocket*> SocketMap;
    SocketMap socketMap;
  public:
    /**
     * Constructor.
     */
    TCPSocketMap() {}

    /**
     * Destructor.
     */
    ~TCPSocketMap() {}

    /**
     * Finds the socket (by connId) for the given message. The message
     * must have arrived from TCPMain, and must contain a TCPCommand
     * control info object. The method returns NULL if the socket was
     * not found, and throws an error if the message doesn't contain
     * a TCPCommand.
     */
    TCPSocket *findSocketFor(cMessage *msg);

    /**
     * Registers the given socket. Should not be called multiple times
     * for one socket object.
     */
    void addSocket(TCPSocket *socket);

    /**
     * Removes the given socket from the data structure.
     */
    TCPSocket *removeSocket(TCPSocket *socket);

    /**
     * Returns the number of sockets stored.
     */
    unsigned int size() {return socketMap.size();}
};

#endif


