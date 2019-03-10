#!/usr/bin/env python3
# Phoenix-RTOS
#
# Connect on port, send some data, close connection
#
# Copyright 2019 Phoenix Systems
# Author: Bartosz Ciesla
#
# This file is part of Phoenix-RTOS.

from twisted.internet import reactor
from twisted.internet.protocol import Protocol
from twisted.internet.endpoints import TCP4ClientEndpoint, connectProtocol
from twisted.internet.error import ConnectionDone
import sys
import random

verbose = False

def printUsage(progname):
    print("Usage: {0} IP_ADDRESS_1:PORT_1A[:PORT_1B[...]] ITERATIONS_1 [IP_ADDRESS_2:PORT_2A[:PORT_2B[...]] ITERATIONS_2 [...]]\n".format(progname))
    print("Examples:")
    print("\t{0} 10.255.10.31:22 10\n\t\tmake 10 connections on 10.255.10.31 port 22".format(progname))
    print("\t{0} 10.255.10.31:22 10 10.255.10.32:22 20\n\t\tmake 10 connections on 10.255.10.31 port 22 and 20 connections on 10.255.10.32 port 22".format(progname))
    print("\t{0} 10.255.10.31:22:80:443 15 10.255.10.32:21:22 100\n\t\tmake 15 connections per port (22, 80 and 443) on 10.255.10.31 and 100 connections per port (21 and 22) on 10.255.10.32".format(progname))

def debug_print(message):
    if verbose == True:
        print("DEBUG: {0}".format(message))

class SocketTestSetup:
    def __init__(self, ipAddress, port, maxIterations, disconnectTime = -1, connectTime = -1):
        self.ipAddress = ipAddress
        self.port = port
        self.connectionEstablished = 0
        self.connectionError = maxIterations # will be decremented on success
        self.connectionDone = 0
        self.connectionLost = 0
        self.iterations = 0
        self.maxIterations = maxIterations
        self.disconnectTime = disconnectTime # time to disconnect socket
        self.connectTime = connectTime       # time between subsequent connections
        self.maxRandom = 10
        self.lastConnectionTime = 0          # holds value (in seconds) of last connection attempt when evets were already scheduled

    def __str__(self):
        return "Address {0}:{1}\nOpen success: {2}\nOpen/send error: {3}\nClose success: {4}\nConnection lost: {5}".format(
                self.ipAddress, self.port, self.connectionEstablished, self.connectionError, self.connectionDone, self.connectionLost)

    def getEffectiveDisconnectTime(self):
        effectiveDisconnectTime = 0
        if self.disconnectTime < 0:
            effectiveDisconnectTime = random.randint(0, self.maxRandom)
        else:
            effectiveDisonnectTime = self.disconnectTime
        return effectiveDisonnectTime

    def getEffectiveConnectTime(self):
        effectiveConnectTime = 0
        if self.connectTime < 0:
            effectiveConnectTime = random.randint(0, self.maxRandom)
        else:
            effectiveConnectTime = self.connectTime
        return effectiveConnectTime

    def scheduleEvents(self):
        delay = 0
        for i in range(self.maxIterations):
            delay += self.getEffectiveConnectTime()
            reactor.callLater(delay, makeConnection, self)
        self.lastConnectionTime = delay

    def getLastConnectionEndTime(self):
        if self.disconnectTime < 0:
            return self.lastConnectionTime + self.maxRandom
        else:
            return self.lastConnectionTime + self.disconnectTime


def makeConnection(testSetup):
    testSetup.iterations += 1
    print("{0}:{1} ({2}/{3})".format(testSetup.ipAddress, testSetup.port, testSetup.iterations, testSetup.maxIterations))
    point = TCP4ClientEndpoint(reactor, testSetup.ipAddress, testSetup.port)
    d = connectProtocol(point, SocketTestProtocol(testSetup))
    d.addCallback(gotProtocol)
    d.addErrback(handleError)
    d.addCallback(countError)
    d.addErrback(handleError)

# Determine test duration and add reactor stop event
def scheduleStopEvent(testSetups):
    connectionEndTimes = [0]
    for setup in testSetups:
        connectionEndTimes.append(setup.getLastConnectionEndTime())
    maxTestDuration = max(connectionEndTimes) + 1
    reactor.callLater(maxTestDuration, reactor.stop)
    print("Maximum test duration: {0} seconds".format(maxTestDuration))

class SocketTestProtocol(Protocol):
    def __init__(self, testSetup):
        self.testSetup = testSetup

    def sendMessage(self, msg):
        debug_print("Sending message... '{0}'".format(msg))
        self.transport.write(msg.encode("utf-8"))
        debug_print("Sent message")

    def disconnect(self):
        debug_print("Disconnecting...")
        self.transport.loseConnection()
        debug_print("Disconnected")

    def connectionLost(self, reason):
        if reason.type == ConnectionDone:
            self.testSetup.connectionDone += 1
            # Clean close
            debug_print("Connection closed cleanly")
        else:
            self.testSetup.connectionLost += 1
            debug_print("Conntection lost: '{0}'".format(reason))

def gotProtocol(p):
    p.testSetup.connectionEstablished += 1
    p.sendMessage("TEST: on connect message") # It really doesn't matter what is sent here
    effectiveConnectTime = 0
    if p.testSetup.connectTime < 0:
        effectiveDisconnectTime = random.randint(0, p.testSetup.maxRandom)
    else:
        effectiveDisconnectTime = p.testSetup.disconnectTime
    reactor.callLater(effectiveDisconnectTime, p.disconnect)
    # Return protocol to distingush between success and error in later callbacks
    return p

def handleError(failure):
    # Nothing to do
    debug_print("ERROR: {0}".format(failure))

def countError(p):
    # When error occured return type would other than SocketTestProtocol
    if type(p) == SocketTestProtocol:
        p.testSetup.connectionError -= 1

if __name__ == "__main__":
    if (len(sys.argv) < 3) or (len(sys.argv) % 2 == 0):
        print("Not enough arguments or number of arguments is not even")
        printUsage(str(sys.argv[0]))
        sys.exit(-1)

    random.seed()
    testSetups = []
    for i in range(1, len(sys.argv), 2):
        ipAddressAndPorts = sys.argv[i].split(":")
        ipAddress = ipAddressAndPorts[0]
        iterations = int(sys.argv[i + 1])

        for idx in range(1, len(ipAddressAndPorts)):
            port = int(ipAddressAndPorts[idx])
            testSetups.append(SocketTestSetup(ipAddress, port, iterations))

    for setup in testSetups:
        setup.scheduleEvents()

    scheduleStopEvent(testSetups)
    reactor.run()

    for setup in testSetups:
        print("\n{0}".format(setup))

