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
    resp = await bz.create_db(uuid)
    pprint(json.loads(resp.get_result()))
    pprint("here1")
    mydb = resp.get_db()
    pprint("here2")


    resp = await bz.has_db(uuid)
    pprint(resp)
    # mydb = resp.get_db()
    # resp = await mydb.create("mykey", "myvalue")
    # json_resp = resp.get_result()
    # if (json_resp['result'] == 1):
    #     print(json_resp)
    # resp = await mydb.read("mykey")
    # if (json_resp['result'] == 1):
    #     print("Value == ", json_resp['value'])
    bzpy.terminate()
    pprint("here3")
#asyncio.run(create_and_check(uuid))
#os.getpid()

bzpy.set_string("a")

bzpy.initialize(pub_key, priv_key, "ws://127.0.0.1:50000")
res = bzpy.create_db(uuid)
time.sleep(2)
print("\ncreate_db:\n")
pprint(json.loads(res.get_result()))
res = bzpy.has_db(uuid)
time.sleep(2)
print("\nhas_db:\n")
pprint(json.loads(res.get_result()))

res = bzpy.open_db(uuid)
time.sleep(2)
print("\nopen_db:\n")
pprint(res.get_result())

mydb = res.get_db()
bzpy.terminate()
res = mydb.create("akey", "aval")
time.sleep(2)
pprint(json.loads(res.get_result()))

