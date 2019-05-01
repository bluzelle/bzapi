from pprint import pprint
import asyncio
from lib.udp.udp_support import *
from build.library import bzpy
import json

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

        self.udp_open = False

    async def load_(self, *args, **kwargs):
        if not self.udp_open:
            self.local = await open_local_endpoint(self.localhost_ip, self.async_udp_port)
            self.udp_open = True
        method_handle = getattr(kwargs['obj'], kwargs['meth'])
        resp = method_handle(*args[1:])
        resp.get_signal_id(self.async_udp_port)
        data, address = await self.local.receive()
        self.local.close()
        self.udp_open = False
        return resp


    async def create_db(self, uuid):
        response = await self.load_(self, uuid, obj = bzpy, meth = self.create_db.__name__)
        return response

    async def open_db(self, uuid):
        response = await self.load_(self, uuid, obj = bzpy, meth = self.open_db.__name__)
        results = json.loads(response.get_result())
        results.get_db = response.get_db
        return results

    async def has_db(self, uuid):
        response = await self.load_(self, uuid, obj = bzpy, meth = self.has_db.__name__)
        results = json.loads(response.get_result())
        return results['result'] == 1

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