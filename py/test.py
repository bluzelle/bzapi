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


    print("starting has_db ... ")

    resp = await bz.has_db(uuid)
    pprint(resp)
    print("has_db finished ... rusult = ", resp)

    print("starting open_db ... ")
    db = await bz.open_db(uuid)
    print("open_db finished ... db_obj = ", db)

    create = await db.create("akey", "aval")

    print("db.create finished ... db_obj = ", create)

    print("starting bzpy.terminate() ")
    bzpy.terminate()
    print("finish")



asyncio.run(create_and_check(uuid))
# bzpy.initialize(pub_key, priv_key, "ws://127.0.0.1:50000")


# res = bzpy.open_db(uuid)
# time.sleep(2)
# print("\nopen_db:\n")
# pprint(res.get_result())
#
# mydb = res.get_db()
#
# res = mydb.create("akey", "aval")
# time.sleep(2)
# pprint(json.loads(res.get_result()))
#
#
# res = mydb.read("akey")
# time.sleep(2)
# print("AA ", json.loads(res.get_result()))
#
# res = mydb.update("akey", "aval2!")
# time.sleep(2)
# print("BB", json.loads(res.get_result()))
#
#
# res = mydb.read("akey")
# time.sleep(2)
# print("CC ", json.loads(res.get_result()))
#
# res = mydb.remove("akey")
# time.sleep(2)
# print("DD ", json.loads(res.get_result()))
#
# res = mydb.read("akey")
# time.sleep(2)
# print("EE ", json.loads(res.get_result()))
#
#
# bzpy.terminate()
