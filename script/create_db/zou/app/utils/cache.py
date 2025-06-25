"""
This module is a wrapper for flask_caching. It configures it and rename
the memoize function. The aim with that cache is to minimize the requests
made on the target database.
"""

import redis

from flask_caching import Cache
from zou.app import config


def invalidate(*args):
    return


def clear():
    return
