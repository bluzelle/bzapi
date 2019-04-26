from udp.udp_support import *
from build.library import bzpy


class Bluzelle:
    def __init__(self, pub_key, priv_key):
        self.priv_key = priv_key
        self.pub_key = pub_key
        if (not bzpy.initialize(pub_key, priv_key, "ws://127.0.0.1:50000")):
            raise Exception('Could not run initialize')

    async def has_db(uuid):
        my_port = 1234
        local = await open_local_endpoint('127.0.0.1', my_port)
        resp = bzpy.has_db(uuid)
        cpp_port = resp.get_signal_id(my_port)

        data, address = await local.receive() # wait for processing to continue
