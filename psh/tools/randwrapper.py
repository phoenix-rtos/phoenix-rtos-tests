# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# Simple random module wrapper intended to use in tests
#
# Copyright 2022 Phoenix Systems
# Author: Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import random


class TestRandom:
    """ Simple random module wrapper intended to use in tests

    This is only source of randomness that can be used in test cases. It needs to be
    explicitly initialized with constant seed.

    Current implementation is using random.Random. Note that python standard
    library guarantees reproducibility only for `random` method. If it turns out
    to be a problem TestRandom methods can be modified to fix this.
    """
    def __init__(self, seed):
        self.rand_obj = random.Random()
        if seed is None:
            raise Exception("Please don't use None seed, the current time initialization is not supported.")
        self.rand_obj.seed(a=seed)

    def randint(self, a, b):
        """ Return random integer in range [a, b], including both end points. """
        return self.rand_obj.randint(a, b)

    def choices(self, population, k=1):
        """ Return a k sized list of population elements chosen with replacement. """
        return self.rand_obj.choices(population, k=k)
