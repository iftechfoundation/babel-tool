#!/bin/bash

# Here we have several strings that are almost UUIDs, but don't match
# the format. Only the last one counts.

# Dollar sign is no good.
# UUID://ABCDE-12345-$-54321//

# Lower-case is no good
# UUID://26df0a0a-13fa-4c1d-9f9a-b8d988ea9f34//

# One slash on the end, should be two
# UUID://720B7596-4D32-4F79-AA8E-FAA7681DF5B4/

# Spaces in the middle is bad
# UUID://78D1986A-C0A7-48E0   9E52-A16D0ADFE992//

# This is correct!
# UUID://7590E1B3-9076-4FEE-9941-57D8F4515BB2//

echo 'Hunt the Shwumpus!'

echo '...You have died.'
