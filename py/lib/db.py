from pprint import pprint
import asyncio
import sys
import json

from build.library import bzpy
from lib.udp.udp_support import *
from lib.udp.test_udp import *

class DB:

    def __init__(self, cpp_db):
        self.localhost_ip = "127.0.0.1"
        self.cpp_db = cpp_db

    def load_(self, *args, **kwargs):
        method_handle = getattr(self.cpp_db, kwargs['meth'])
        resp = method_handle(*args[1:])
        return resp

    def create(self, *args, **kwargs):
        results = json.loads(self.load_(self, *args, **kwargs, meth = sys._getframe().f_code.co_name))
        if 'result' in results:
            return results['result'] == 1
        else:
            return False

    def update(self, *args, **kwargs):
        response = self.load_(self, *args, **kwargs, meth = sys._getframe().f_code.co_name)
        results = json.loads(response)
        if 'result' in results:
            return results['result'] == 1
        else:
            return False

    def remove(self, *args, **kwargs):
        response = self.load_(self, *args, **kwargs, meth = sys._getframe().f_code.co_name)
        results = json.loads(response)
        if 'result' in results:
            return results['result'] == 1
        else:
            return False

    def has(self, *args, **kwargs):
        response = self.load_(self, *args, **kwargs, meth = sys._getframe().f_code.co_name)
        results = json.loads(response)
        if 'result' in results:
            return results['result'] == 1
        else:
            return False

    def read(self, *args, **kwargs):
        return json.loads(self.load_(self, *args, **kwargs, meth = sys._getframe().f_code.co_name))

    def quick_read(self, *args, **kwargs):
        response = self.load_(self, *args, **kwargs, meth = sys._getframe().f_code.co_name)
        results = json.loads(response)
        return results

    def expire(self, *args, **kwargs):
        response = self.load_(self, *args, **kwargs, meth = sys._getframe().f_code.co_name)
        results = json.loads(response)
        return results

    def persist(self, *args, **kwargs):
        response = self.load_(self, *args, **kwargs, meth = sys._getframe().f_code.co_name)
        results = json.loads(response)
        return results

    def ttl(self, *args, **kwargs):
        response = self.load_(self, *args, **kwargs, meth = sys._getframe().f_code.co_name)
        results = json.loads(response)
        return results

    def keys(self):
        response = self.load_(self, meth = sys._getframe().f_code.co_name)
        results = json.loads(response)
        return results['keys']

    def size(self):
        response = self.load_(self, meth = sys._getframe().f_code.co_name)
        results = json.loads(response)
        return results


    def swarm_status(self):
        response = self.cpp_db.swarm_status()
        return response