#!/bin/bash -x

# Copyright(c) 2015-2017 Intel Corporation. All rights reserved.
# 
# This program is free software; you can redistribute it and/or modify it
# under the terms of version 2 of the GNU General Public License as
# published by the Free Software Foundation.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

set -e

SECTOR_SIZE="4096"
rc=77

. ./common

check_min_kver "4.5" || do_skip "may lack namespace mode attribute"

trap 'err $LINENO' ERR

# setup (reset nfit_test dimms)
modprobe nfit_test
$NDCTL disable-region -b $NFIT_TEST_BUS0 all
$NDCTL zero-labels -b $NFIT_TEST_BUS0 all
$NDCTL enable-region -b $NFIT_TEST_BUS0 all

rc=1

# create pmem
dev="x"
json=$($NDCTL create-namespace -b $NFIT_TEST_BUS0 -t pmem -m raw)
eval $(echo $json | json2var )
[ $dev = "x" ] && echo "fail: $LINENO" && exit 1
[ $mode != "raw" ] && echo "fail: $LINENO" &&  exit 1

# convert pmem to fsdax mode
json=$($NDCTL create-namespace -m fsdax -f -e $dev)
eval $(echo $json | json2var)
[ $mode != "fsdax" ] && echo "fail: $LINENO" &&  exit 1

# convert pmem to sector mode
json=$($NDCTL create-namespace -m sector -l $SECTOR_SIZE -f -e $dev)
eval $(echo $json | json2var)
[ $sector_size != $SECTOR_SIZE ] && echo "fail: $LINENO" &&  exit 1
[ $mode != "sector" ] && echo "fail: $LINENO" &&  exit 1

# free capacity for blk creation
$NDCTL destroy-namespace -f $dev

# create blk
dev="x"
json=$($NDCTL create-namespace -b $NFIT_TEST_BUS0 -t blk -m raw -v)
eval $(echo $json | json2var)
[ $dev = "x" ] && echo "fail: $LINENO" && exit 1
[ $mode != "raw" ] && echo "fail: $LINENO" &&  exit 1

# convert blk to sector mode
json=$($NDCTL create-namespace -m sector -l $SECTOR_SIZE -f -e $dev)
eval $(echo $json | json2var)
[ $sector_size != $SECTOR_SIZE ] && echo "fail: $LINENO" &&  exit 1
[ $mode != "sector" ] && echo "fail: $LINENO" &&  exit 1

_cleanup

exit 0
