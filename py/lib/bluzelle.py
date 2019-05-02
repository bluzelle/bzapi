from pprint import pprint
import asyncio
import sys
from lib.udp.udp_support import *
from build.library import bzpy
import json
from lib.db import DB

class Bluzelle:

    def __init__(self, pub_key, priv_key, ws_address="127.0.0.1", ws_port="50000"):
        self.localhost_ip = "127.0.0.1"
        self.async_udp_port = 1234
        self.ws_address = ws_address
        self.ws_port = ws_port
        self.priv_key = priv_key
        self.pub_key = pub_key
        full_url = f"ws://{ws_address}:{ws_port}"
        if (not bzpy.initialize(pub_key, priv_key, full_url)):
            raise Exception('Could not run initialize')

    async def load_(self, *args, **kwargs):
        self.async_udp_port = self.async_udp_port + 1
        self.local = await open_local_endpoint(self.localhost_ip, self.async_udp_port)
        method_handle = getattr(kwargs['obj'], kwargs['meth'])
        resp = method_handle(*args[1:])
        resp.get_signal_id(self.async_udp_port)
        data, address = await self.local.receive()
        self.local.close()
        return resp


    async def create_db(self, uuid):
        response = await self.load_(self, uuid, obj = bzpy, meth = sys._getframe().f_code.co_name)
        results = json.loads(response.get_result())
        if 'error' in results:
            raise Exception(results['error'])
        else:
            return DB(response)


    async def has_db(self, uuid):
        response = await self.load_(self, uuid, obj = bzpy, meth = sys._getframe().f_code.co_name)
        results = json.loads(response.get_result())
        return results['result'] == 1

    async def open_db(self, uuid):
        response = await self.load_(self, uuid, obj = bzpy, meth = sys._getframe().f_code.co_name)
        results = json.loads(response.get_result())
        if 'error' in results:
            raise Exception(results['error'])
        else:
            return DB(response)



        #
    #
    # async def load_(self, *args, **kwargs):
    #     local = await open_local_endpoint(self.localhost_ip, self.async_udp_port)
    #     method_handle = getattr(kwargs['obj'], kwargs['meth'])
    #     uft8_args = list()
    #     for i, arg in enumerate(args[1:]):
    #         if (isinstance(arg, str)):
    #             uft8_args.append(arg.encode('utf-8'))
    #         else:
    #             uft8_args.append(arg)
    #     t = tuple(uft8_args)
    #     resp = method_handle(*t)
    #     resp.get_signal_id(self.async_udp_port)
    #     data, address = await local.receive()
    #     return resp