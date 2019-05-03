from pprint import pprint
import asyncio
import sys
from lib.udp.udp_support import *
from build.library import bzpy
import json


class DB:

    def __init__(self, response):
        self.localhost_ip = "127.0.0.1"
        self.async_udp_port = 9234
        self.response = response
        self.cpp_db = response.get_db()

    async def load_(self, *args, **kwargs):
        self.async_udp_port = self.async_udp_port + 1
        self.local = await open_local_endpoint(self.localhost_ip, self.async_udp_port)
        method_handle = getattr(self.cpp_db, kwargs['meth'])
        resp = method_handle(*args[1:])
        resp.get_signal_id(self.async_udp_port)
        data, address = await self.local.receive()
        self.local.close()
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

