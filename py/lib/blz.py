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
        if (not bzpy.initialize(pub_key, priv_key, f"ws://{ws_address}:{ws_port}")):
            raise Exception('Could not run initialize')
        self.curr_db = None

    async def load_(self, *args, **kwargs):
        local = await open_local_endpoint(self.localhost_ip, self.async_udp_port)
        method_handle = getattr(kwargs['obj'], kwargs['meth'])
        resp = method_handle(*args[1:])
        resp.get_signal_id(self.async_udp_port)
        data, address = await local.receive()
        return resp

    async def create_db(self, uuid):
        response = await self.load_(self, uuid, obj = bzpy, meth = self.create_db.__name__)
        self.curr_db = response.get_db()
        results = json.loads(resp.get_result())
        return results['result'] == 1

    async def open_db(self, uuid):
        response = await self.load_(self, uuid, obj = bzpy, meth = self.open_db.__name__)
        self.curr_db = response.get_db()
        results = json.loads(resp.get_result())
        return results['result'] == 1

    async def has_db(self, uuid):
        resp = await self.load_(self, uuid, obj = bzpy, meth = self.has_db.__name__)
        results = json.loads(resp.get_result())
        return results['result'] == 1

    std::shared_ptr<response> create(const bzapi::key_t& key, const bzapi::value_t& value);
    std::shared_ptr<response> read(const bzapi::key_t& key);
    std::shared_ptr<response> update(const bzapi::key_t& key, const bzapi::value_t& value);
    std::shared_ptr<response> remove(const bzapi::key_t& key);

    std::shared_ptr<response> quick_read(const bzapi::key_t& key);
    std::shared_ptr<response> has(const bzapi::key_t& key);
    std::shared_ptr<response> keys();
    std::shared_ptr<response> size();
    std::shared_ptr<response> expire(const bzapi::key_t& key, bzapi::expiry_t expiry);
    std::shared_ptr<response> persist(const bzapi::key_t& key);
    std::shared_ptr<response> ttl(const bzapi::key_t& key);