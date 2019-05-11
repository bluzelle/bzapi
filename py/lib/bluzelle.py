from pprint import pprint
import asyncio
import sys
import json

from ecdsa import SigningKey
import logging
from build.library import bzpy
from lib.udp.udp_support import *
from lib.db import DB

class Bluzelle:

    def __init__(self, priv_key, ws_address="127.0.0.1", ws_port="50000"):
        self.localhost_ip = "127.0.0.1"
        self.ws_address = ws_address
        self.ws_port = ws_port
        self.priv_key = priv_key
        try:
            pem_priv_key = SigningKey.from_pem(priv_key)
            pem_pub_key = pem_priv_key.get_verifying_key().to_pem().decode("utf-8")
            self.pub_key = pem_pub_key.replace("-----BEGIN PUBLIC KEY-----\n","").replace("\n-----END PUBLIC KEY-----\n","")
        except Exception as e:
            logging.error(f'Error parsing private key {priv_key}. Error: {str(e)}')
            raise Exception(f'Error parsing private key {priv_key}')

        full_url = f"ws://{ws_address}:{ws_port}"

        self.init_happened = False
        if (not bzpy.initialize(self.pub_key, self.priv_key, full_url)):
            raise Exception('Could not run initialize the Bluzelle object')
        else:
            self.init_happened = True

        self.datagram_endpoint = None
        self.transport = None

    def __del__(self):
        if (self.init_happened):
            bzpy.terminate()

    def load_(self, *args, **kwargs):
        method_handle = getattr(bzpy, kwargs['meth']+'_sync')
        resp = method_handle(*args[1:])
        return resp

    def create_db(self, *args, **kwargs):
        response = self.load_(self, *args, **kwargs, meth = sys._getframe().f_code.co_name)
        return DB(response)

    def has_db(self, *args, **kwargs):
        return self.load_(self, *args, **kwargs, meth = sys._getframe().f_code.co_name)

    def open_db(self, *args, **kwargs):
        response = self.load_(self, *args, **kwargs, meth = sys._getframe().f_code.co_name)
        return DB(response)