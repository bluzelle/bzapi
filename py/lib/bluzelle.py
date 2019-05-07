from pprint import pprint
import asyncio
import sys
import json

from build.library import bzpy
from lib.udp.udp_support import *
from lib.udp.test_udp import *
from lib.db import DB

class Bluzelle:

    def __init__(self, pub_key, priv_key, ws_address="127.0.0.1", ws_port="50000"):
        self.localhost_ip = "127.0.0.1"
        self.async_udp_port = get_next_free()
        self.ws_address = ws_address
        self.ws_port = ws_port
        self.priv_key = priv_key
        self.pub_key = pub_key
        full_url = f"ws://{ws_address}:{ws_port}"

        self.init_happened = False
        if (not bzpy.initialize(pub_key, priv_key, full_url)):
            raise Exception('Could not run initialize the Bluzelle object')
        else:
            self.init_happened = True

        self.datagram_endpoint = None
        self.transport = None

    def __del__(self):
        if (self.datagram_endpoint):
            self.datagram_endpoint._endpoint.close()
        if (self.transport):
            self.transport.abort()
            self.transport.close()
            self.transport._sock.close()
        if (self.init_happened):
            bzpy.terminate()

    async def load_(self, *args, **kwargs):
        self.async_udp_port = self.async_udp_port
        if not self.datagram_endpoint:
            res = await open_local_endpoint(self.localhost_ip, self.async_udp_port)
            self.datagram_endpoint = res[2]
            self.transport = res[1]
        method_handle = getattr(kwargs['obj'], kwargs['meth'])
        resp = method_handle(*args[1:])
        resp.get_signal_id(self.async_udp_port)
        data, address = await self.datagram_endpoint._endpoint.receive()
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