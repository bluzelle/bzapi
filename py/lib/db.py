from pprint import pprint
import asyncio
from lib.udp.udp_support import *
from build.library import bzpy
import json


class DB:

    def __init__(self, cpp_db, ws_address="127.0.0.1", ws_port="50000"):
        self.localhost_ip = "127.0.0.1"
        self.async_udp_port = 1234
        self.cpp_db = cpp_db

    async def load_(self, *args, **kwargs):
        local = await open_local_endpoint(self.localhost_ip, self.async_udp_port)
        method_handle = getattr(self.cpp_db, kwargs['meth'])
        resp = method_handle(*args[1:])
        response.get_signal_id(self.async_udp_port)
        data, address = await local.receive()
        return resp

    async def create(self, key, value):
        response = await self.load_(self, key, value, obj = self, meth = self.create.__name__)
        results = json.loads(response.get_result())
        return results['result'] == 1

    async def read(self, key):
        response = await self.load_(self, key, obj = self, meth = self.read.__name__)
        results = json.loads(response.get_result())
        return results

    async def quick_read(self, key):
        response = await self.load_(self, key, obj = self, meth = self.quick_read.__name__)
        results = json.loads(response.get_result())
        return results

    async def update(self, key, value):
        response = await self.load_(self, key, value, obj = self, meth = self.update.__name__)
        results = json.loads(response.get_result())
        return results['result'] == 1

    async def remove(self, key):
        response = await self.load_(self, key, obj = self, meth = self.remove.__name__)
        results = json.loads(response.get_result())
        return results['result'] == 1

    async def has(self, key):
        response = await self.load_(self, key, obj = self, meth = self.has.__name__)
        results = json.loads(response.get_result())
        return results['result'] == 1

    async def keys(self, key):
        response = await self.load_(self, key, obj = self, meth = self.keys.__name__)
        results = json.loads(response.get_result())
        return results

    async def size(self, key):
        response = await self.load_(self, key, obj = self, meth = self.size.__name__)
        results = json.loads(response.get_result())
        return results

    async def expire(self, key, expiry):
        response = await self.load_(self, key, expiry, obj = self, meth = self.expire.__name__)
        results = json.loads(response.get_result())
        return results

    async def persist(self, key):
        response = await self.load_(self, key, obj = self, meth = self.persist.__name__)
        results = json.loads(response.get_result())
        return results

    async def ttl(self, key):
        response = await self.load_(self, key, obj = self, meth = self.ttl.__name__)
        results = json.loads(response.get_result())
        return results

    async def swarm_status(self):
        response = await self.load_(self, obj = self, meth = self.swarm_status.__name__)
        return response

