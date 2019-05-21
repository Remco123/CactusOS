#ifndef __LIBCACTUSOS__IPC_H
#define __LIBCACTUSOS__IPC_H

namespace LIBCactusOS
{
    #define IPC_MAX_ARGS 5

    #define IPC_TYPE_GUI 1

    struct IPCMessage
    {
        int source; //Who has sended this message?
        int dest; //Who is it for
        
        int type; //What type of message is it?

        //Arguments
        unsigned int arg1;
        unsigned int arg2;
        unsigned int arg3;
        unsigned int arg4;
        unsigned int arg5;
        unsigned int arg6;
    };

    /**
     * Send a message to a other process
    */
    int IPCSend(int dest, int type, unsigned int arg1 = 0, unsigned int arg2 = 0, unsigned int arg3 = 0, unsigned int arg4 = 0, unsigned int arg5 = 0, unsigned int arg6 = 0);
    /**
     * Send a message to a other process
    */
    int IPCSend(IPCMessage message);

    /**
     * How many messages are ready for receiving?
    */
    int IPCAvailible();

    /**
     * Receive a single IPCMessage, blocks if none availible
     * FromID: Only receive a message from specified process
    */
    IPCMessage ICPReceive(int fromID = -1, int* errOut = 0);
}

#endif