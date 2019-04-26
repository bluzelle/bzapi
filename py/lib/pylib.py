from pprint import pprint
import asyncio
from lib.udp.udp_support import *
from build.library import bzpy
import json


class Bluzelle:
    def __init__(self, pub_key, priv_key):
        self.priv_key = priv_key
        self.pub_key = pub_key
        if (not bzpy.initialize(pub_key, priv_key, "ws://127.0.0.1:50000")):
            raise Exception('Could not run initialize')

    async def has_db(self, uuid):
        my_port = 1234
        local = await open_local_endpoint('127.0.0.1', my_port)
        resp = bzpy.has_db(uuid)
        cpp_port = resp.get_signal_id(my_port)

        data, address = await local.receive() # wait for processing to continue
        return json.loads(resp.get_result())
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