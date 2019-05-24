import unittest
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
from build.library import bzapi

import logging

class PyLogger(bzapi.logger):

    def __init__(self):
        bzapi.logger.__init__(self)

        # Configure Python logging module root logger
        logging.basicConfig(format='%(asctime)s  %(levelname)s: %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p',
                            level=logging.INFO)

    def log(self, level, message):
        logging.log(level, message)
        print(level, message)


if __name__ == '__main__':

    logger = PyLogger()
    bzapi.set_logger(logger)