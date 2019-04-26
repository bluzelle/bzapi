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

from udp_support import *

sys.path.extend([os.getcwd()])

from build.library import bzpy

pub_key = "MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEiykQ5A02u+02FR1nftxT5VuUdqLO6lvN\n" \
"oL5aAIyHvn8NS0wgXxbPfpuqUPpytiopiS5D+t2cYzXJn19MQmnl/g=="


priv_key = "-----BEGIN EC PRIVATE KEY-----\n" \
           "MHQCAQEEIBWDWE/MAwtXaFQp6d2Glm2Uj7ROBlDKFn5RwqQsDEbyoAcGBSuBBAAK\n" \
           "oUQDQgAEiykQ5A02u+02FR1nftxT5VuUdqLO6lvNoL5aAIyHvn8NS0wgXxbPfpuq\n" \
           "UPpytiopiS5D+t2cYzXJn19MQmnl/g==\n" \
           "-----END EC PRIVATE KEY-----"

async def get_via_socket():
    my_port = 1234
    local = await open_local_endpoint('127.0.0.1', my_port)
    a = bzpy.initialize(pub_key, priv_key, "ws://127.0.0.1:50000")  # EC keys in string form. pub key doesn't have header, private does. see library_test.cpp for example
    print("Init good ", a)
    resp = bzpy.has_db("1111-2222")
    cpp_port = resp.get_signal_id(my_port)
    print("Cpp port", cpp_port)
    print("Result ready 1 ", resp.is_ready())
    data, address = await local.receive() # wait for processing to continue
    print("\nResult ready 2 ", resp.is_ready())
    print("\nResult", resp.get_result())




    # await resp
    # mydb = resp.get_db()
    # resp = mydb.create("mykey", "myvalue")
    # await resp
    # json_resp = resp.get_result()
    # if (json_resp['result'] == 1):
    #     pass
    #     # do stuff
    # resp = mydb.read("mykey")
    # await resp
    # if (json_resp['result'] == 1)
    #     value = json_resp['value']


    # # Create a local UDP enpoint


    # print(f"Got {data!r} from {address[0]} port {address[1]}")
    #
    # print("CPP result ready? ", response.is_ready())
    # print("CPP response => ", response.get_result())

asyncio.run(get_via_socket())