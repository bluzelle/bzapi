from pprint import pprint
import sys
import json
import os
import os.path
import time
sys.path.extend([os.getcwd()])
sys.path.extend([os.path.abspath(os.path.join(os.getcwd(), os.pardir))])
import asyncio
from socket import *
from lib import bluzelle
import uuid
from lib.udp.udp_support import *
from build.library import bzpy

pub_key = "MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEiykQ5A02u+02FR1nftxT5VuUdqLO6lvN\n" \
"oL5aAIyHvn8NS0wgXxbPfpuqUPpytiopiS5D+t2cYzXJn19MQmnl/g=="


priv_key = "-----BEGIN EC PRIVATE KEY-----\n" \
           "MHQCAQEEIBWDWE/MAwtXaFQp6d2Glm2Uj7ROBlDKFn5RwqQsDEbyoAcGBSuBBAAK\n" \
           "oUQDQgAEiykQ5A02u+02FR1nftxT5VuUdqLO6lvNoL5aAIyHvn8NS0wgXxbPfpuq\n" \
           "UPpytiopiS5D+t2cYzXJn19MQmnl/g==\n" \
           "-----END EC PRIVATE KEY-----"

uuid = str(uuid.uuid4())

async def create_and_check(uuid):
    bz = bluzelle.Bluzelle(pub_key, priv_key)

    print("starting create_db ... ")
    db = await bz.create_db(uuid)
    print("create_db finished ... db_obj = ", db)

    resp = await bz.has_db(uuid)
    pprint(resp)
    print("has_db finished ... rusult = ", resp)

    print("starting open_db ... ")
    db = await bz.open_db(uuid)
    print("open_db finished ... db_obj = ", db)

    res = await db.read("akey")
    print("db.read finished ... res = ", res)

    res = await db.persist("key")
    print("db.persist finished ... res = ", res)

    res = await db.create("akey", "aval")
    print("db.create finished ... res = ", res)

    res = await db.has("akey")
    print("db.has finished ... res = ", res)

    res = await db.read("akey")
    print("db.read finished ... res = ", res)

    res = await db.quick_read("akey")
    print("db.quick_read finished ... res = ", res)

    res = await db.ttl("key")
    print("db.ttl finished ... res = ", res)

    res = await db.persist("key")
    print("db.persist finished ... res = ", res)

    res = await db.update("akey", "aval1")
    print("db.update finished ... res = ", res)

    res = await db.read("akey")
    print("db.read finished ... res = ", res)

    res = await db.keys()
    print("db.keys finished ... res = ", res)

    size = await db.size()
    print("db.size finished ... res = ", size)

    res = await db.remove("akey")
    print("db.remove finished ... res = ", res)

    res = await db.keys()
    print("db.keys finished ... res = ", res)

    has = await db.has("akey")
    print("db.has finished ... res = ", has)

    res = await db.keys()
    print("db.keys finished ... res = ", res)

    # res = await db.expire("key", 5)
    # print("db.expire finished ... res = ", res)

    res = await db.swarm_status()
    print("db.swarm_status finished ... res = ", res)

    print("starting bzpy.terminate() ")
    bzpy.terminate()
    print("finish")


loop = asyncio.get_event_loop()
loop.run_until_complete(create_and_check(uuid))
loop.close()