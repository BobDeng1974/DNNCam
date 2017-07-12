#!/usr/bin/env python

from pymodbus.client.sync import ModbusTcpClient as ModbusClient
import sys
import logging
logging.basicConfig()
log = logging.getLogger()

if len(sys.argv) >= 2:
    host = sys.argv[1]
else:
    host = '192.168.192.141'

client = ModbusClient(host, port=5020)
client.connect()

ADDRESSES = { 0 : 'Focus Home',
        1 : 'Focus Absolute',
        2 : 'Focus Relative',
        3 : 'Focus Location',
        10 : 'Zoom Home',
        11 : 'Zoom Absolute',
        12 : 'Zoom Relative',
        13 : 'Zoom Location',
        20 : 'Iris Home',
        21 : 'Iris Absolute',
        22 : 'Iris Relative',
        23 : 'Irirs Location',
        }

def print_app_state():
    for addr, name in sorted(ADDRESSES.items()):
        rr = client.read_holding_registers(addr, 1)
        print "%40s --> %s" % (name, rr.registers[0])

print_app_state()

