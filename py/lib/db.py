from pprint import pprint
import asyncio
import sys
import json

from build.library import bzpy
from lib.udp.udp_support import *
from lib.udp.test_udp import *

class DB:

    def __init__(self, response):
        self.localhost_ip = "127.0.0.1"
        self.async_udp_port = get_next_free()
        self.response = response
        self.cpp_db = response.get_db()
        self.datagram_endpoint = None
        self.transport = None

        def __del__(self):
            if (self.datagram_endpoint):
                self.datagram_endpoint._endpoint.close()
            if (self.transport):
                self.transport.abort()
                self.transport.close()
                self.transport._sock.close()

    async def load_(self, *args, **kwargs):
        self.async_udp_port = self.async_udp_port
        if not self.datagram_endpoint:
            res = await open_local_endpoint(self.localhost_ip, self.async_udp_port)
            self.datagram_endpoint = res[2]
            self.transport = res[1]
        method_handle = getattr(self.cpp_db, kwargs['meth'])
        resp = method_handle(*args[1:])
        resp.get_signal_id(self.async_udp_port)
        data, address = await self.datagram_endpoint._endpoint.receive()
        return resp

    async def create(self, key, value):
        response = await self.load_(self, key, value, obj = self, meth = sys._getframe().f_code.co_name)
        results = json.loads(response.get_result())
        return results['result'] == 1

    async def read(self, key):
        response = await self.load_(self, key, obj = self, meth = sys._getframe().f_code.co_name)
        results = json.loads(response.get_result())
        return results

    async def quick_read(self, key):
        response = await self.load_(self, key, obj = self, meth = sys._getframe().f_code.co_name)
        results = json.loads(response.get_result())
        return results

    async def update(self, key, value):
        response = await self.load_(self, key, value, obj = self, meth = sys._getframe().f_code.co_name)
        results = json.loads(response.get_result())
        return results['result'] == 1

    async def remove(self, key):
        response = await self.load_(self, key, obj = self, meth = sys._getframe().f_code.co_name)
        results = json.loads(response.get_result())
        return results['result'] == 1

    async def has(self, key):
        response = await self.load_(self, key, obj = self, meth = sys._getframe().f_code.co_name)
        results = json.loads(response.get_result())
        return results['result'] == 1

    async def keys(self):
        response = await self.load_(self, obj = self, meth = sys._getframe().f_code.co_name)
        results = json.loads(response.get_result())
        return results

    async def size(self):
        response = await self.load_(self, obj = self, meth = sys._getframe().f_code.co_name)
        results = json.loads(response.get_result())
        return results

    async def expire(self, key, expiry):
        response = await self.load_(self, key, expiry, obj = self, meth = sys._getframe().f_code.co_name)
        results = json.loads(response.get_result())
        return results

    async def persist(self, key):
        response = await self.load_(self, key, obj = self, meth = sys._getframe().f_code.co_name)
        results = json.loads(response.get_result())
        return results

    async def ttl(self, key):
        response = await self.load_(self, key, obj = self, meth = sys._getframe().f_code.co_name)
        results = json.loads(response.get_result())
        return results

    async def swarm_status(self):
        response = self.response.get_db().swarm_status()
        return response

