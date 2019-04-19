# echo -n "b" | /usr/bin/nc -4u -w0 localhost 1246
from pprint import pprint
import sys
import os
import asyncio
import concurrent.futures
import time
import uvloop
import gc
from socket import *

import asyncio
import warnings

#from udp_support import *

sys.path.extend([os.getcwd()])

from librarya import bzpy

async def get_via_socket():
    my_port = 1246
    # # Create a local UDP enpoint
    # local = await open_local_endpoint('127.0.0.1', my_port)
    # 
    # db = libdb.DB()
    # task = db.newTest()
    # response = task.makeResponseSharedPtr()
    # task.setResponseSharedPtr(response)
    # cpp_port = response.get_signal_id(my_port)
    # print("CPP process_request()..... ")
    #
    # task.process_request(timeout=1) # aka DB.get()
    #
    # print("Python: waiting for UDP signal..... ")
    #
    # data, address = await local.receive() # wait for processing to continue
    #
    # print(f"Got {data!r} from {address[0]} port {address[1]}")
    #
    # print("CPP result ready? ", response.is_ready())
    # print("CPP response => ", response.get_result())

asyncio.run(get_via_socket())