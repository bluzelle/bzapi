from pprint import pprint
import sys
import os
sys.path.extend([os.getcwd()])

import asyncio
from socket import *
from lib import blz

pub_key = "MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEiykQ5A02u+02FR1nftxT5VuUdqLO6lvN\n" \
"oL5aAIyHvn8NS0wgXxbPfpuqUPpytiopiS5D+t2cYzXJn19MQmnl/g=="


priv_key = "-----BEGIN EC PRIVATE KEY-----\n" \
           "MHQCAQEEIBWDWE/MAwtXaFQp6d2Glm2Uj7ROBlDKFn5RwqQsDEbyoAcGBSuBBAAK\n" \
           "oUQDQgAEiykQ5A02u+02FR1nftxT5VuUdqLO6lvNoL5aAIyHvn8NS0wgXxbPfpuq\n" \
           "UPpytiopiS5D+t2cYzXJn19MQmnl/g==\n" \
           "-----END EC PRIVATE KEY-----"

uuid = "11111"

async def check_has_db(uuid):
    bz = blz.Bluzelle(pub_key, priv_key)
    res = await bz.has_db(uuid)
    print("Result:")
    pprint(res)

asyncio.run(check_has_db(uuid))

# await resp
# mydb = resp.get_db()
# resp = mydb.create("mykey", "myvalue")
# await resp
# json_resp = resp.get_result()
# if (json_resp['result'] == 1):
#     pass
#     # do stuff
# resp = mydb.read("mykey")
# await resp
# if (json_resp['result'] == 1)
#     value = json_resp['value']
