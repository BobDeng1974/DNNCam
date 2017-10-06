#!/usr/bin/env python

import logging, sys
logging.basicConfig(stream=sys.stdout, level=logging.INFO, format='%(asctime)s %(levelname)s: %(message)s [%(filename)s:%(lineno)s]')

from pymodbus.server.async import StartTcpServer
from pymodbus.device import ModbusDeviceIdentification
from pymodbus.datastore.store import BaseModbusDataBlock
from pymodbus.datastore import ModbusSlaveContext, ModbusServerContext
from pymodbus.client.sync import ModbusSerialClient
from pymodbus.exceptions import ModbusException

import xmlrpclib
import struct
import os
import time
import subprocess

SERVER="http://localhost:7001/RPC2"

def do_xml_rpc(method, *args):
    s=xmlrpclib.ServerProxy(SERVER)
    ret = getattr(s, method)(*args)
    logging.info("XMLRPC %s => %r" % (method, ret))
    if ret == -1: return 0
    return ret

ADDRESS_MIN=0
ADDRESS_MAX=23
ADDRESSES = {
    0 : ('Focus Home', None, lambda: do_xml_rpc('focus_home')),
    1 : ('Focus Absolute', None, lambda v: do_xml_rpc('focus_absolute', v)),
    2 : ('Focus Relative', None, lambda v: do_xml_rpc('focus_relative', v)),
    3 : ('Focus Location', lambda: do_xml_rpc('focus_get_location'), None),

    10 : ('Zoom Home', None, lambda: do_xml_rpc('zoom_home')),
    11 : ('Zoom Absolute', None, lambda v: do_xml_rpc('zoom_absolute', v)),
    12 : ('Zoom Relative', None, lambda v: do_xml_rpc('zoom_relative', v)),
    13 : ('Zoom Location', lambda: do_xml_rpc('zoom_get_location'), None),
    
    20 : ('Iris Home', None, lambda: do_xml_rpc('iris_home')),
    21 : ('Iris Absolute', None, lambda v: do_xml_rpc('iris_absolute', v)),
    22 : ('Iris Relative', None, lambda v: do_xml_rpc('iris_relative', v)),
    23 : ('Iris Location', lambda: do_xml_rpc('iris_get_location'), None)
}

class LensDriverHoldingRegisters(BaseModbusDataBlock):
    ysi_data = None
    def getValue(self, address):
        if address >= ADDRESS_MIN and address <= ADDRESS_MAX:
            if ADDRESSES[address][1]:
                try:
                    return ADDRESSES[address][1]()
                except:
                    import traceback
                    traceback.print_exc()
                    raise
            else:
                return 0

        return address

    def setValue(self, address, value):
        if address >= ADDRESS_MIN and address <= ADDRESS_MAX + 1:
            if ADDRESSES[address][2]:
                ADDRESSES[address][2](value)

    def getValues(self, address, count=1):
        #address -=1
        if address >= ADDRESS_MIN and address + count <= ADDRESS_MAX + 1:
            return [self.getValue(address+i) for i in xrange(count)]

    def setValues(self, address, values):
        #address -=1
        for offset, value in enumerate(values):
            self.setValue(address+offset, value)

    def validate(self, address, count):
        #address -=1
        if address >= ADDRESS_MIN and address + count <= ADDRESS_MAX + 1:
            return True
        return False

#---------------------------------------------------------------------------# 
# initialize your data store
#---------------------------------------------------------------------------# 
hr_block = LensDriverHoldingRegisters()
store = ModbusSlaveContext(di=hr_block, co=hr_block, hr=hr_block, ir=hr_block, zero_mode=True)
context = ModbusServerContext(slaves=store, single=True)

#---------------------------------------------------------------------------# 
# initialize the server information
#---------------------------------------------------------------------------# 
identity = ModbusDeviceIdentification()
identity.VendorName  = 'pymodbus'
identity.ProductCode = 'PM'
identity.VendorUrl   = 'http://github.com/bashwork/pymodbus/'
identity.ProductName = 'pymodbus Server'
identity.ModelName   = 'pymodbus Server'
identity.MajorMinorRevision = '1.0'

#---------------------------------------------------------------------------# 
# run the server you want
#---------------------------------------------------------------------------# 
StartTcpServer(context, identity=identity, address=("0.0.0.0", 5021))

