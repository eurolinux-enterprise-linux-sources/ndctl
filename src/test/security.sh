#!/bin/bash -Ex
# SPDX-License-Identifier: GPL-2.0
# Copyright(c) 2018 Intel Corporation. All rights reserved.

rc=77
dev=""
id=""
keypath="/etc/ndctl/keys"
masterkey="nvdimm-master-test"
masterpath="$keypath/$masterkey"

. ./common

trap 'err $LINENO' ERR

setup()
{
	$NDCTL disable-region -b "$NFIT_TEST_BUS0" all
}

detect()
{
	dev="$($NDCTL list -b "$NFIT_TEST_BUS0" -D | jq -r .[0].dev)"
	[ -n "$dev" ] || err "$LINENO"
	id="$($NDCTL list -b "$NFIT_TEST_BUS0" -D | jq -r .[0].id)"
	[ -n "$id" ] || err "$LINENO"
}

setup_keys()
{
	if [ ! -d "$keypath" ]; then
		mkdir -p "$keypath"
	fi

	dd if=/dev/urandom bs=1 count=32 2>/dev/null | keyctl padd user "$masterkey" @u
	keyctl pipe "$(keyctl search @u user $masterkey)" > "$masterpath"
}

test_cleanup()
{
	if keyctl search @u encrypted nvdimm:"$id"; then
		keyctl unlink "$(keyctl search @u encrypted nvdimm:"$id")"
	fi

	if keyctl search @u user "$masterkey"; then
		keyctl unlink "$(keyctl search @u user $masterkey)"
	fi

	if [ -f "$keypath"/nvdimm_"$id"_"$(hostname)".blob ]; then
		rm -f "$keypath"/nvdimm_"$id"_"$(hostname)".blob
	fi

	if [ -f $masterpath ]; then
		rm -f "$masterpath"
	fi
}

lock_dimm()
{
	$NDCTL disable-dimm "$dev"
	# convert nmemX --> test_dimmY
	# For now this is the only user of such a conversion so we can leave it
	# inline. Once a subsequent user arrives we can refactor this to a
	# helper in test/common:
	#   get_test_dimm_path "nfit_test.0" "nmem3"
	handle="$(ndctl list -b "$NFIT_TEST_BUS0"  -d "$dev" -i | jq -r .[].dimms[0].handle)"
	test_dimm_path=""
	for test_dimm in /sys/devices/platform/"$NFIT_TEST_BUS0"/nfit_test_dimm/test_dimm*; do
		td_handle_file="$test_dimm/handle"
		test -e "$td_handle_file" || continue
		td_handle="$(cat "$td_handle_file")"
		if [[ "$td_handle" -eq "$handle" ]]; then
			test_dimm_path="$test_dimm"
			break
		fi
	done
	test -d "$test_dimm_path"

	# now lock the dimm
	echo 1 > "${test_dimm_path}/lock_dimm"
	sstate="$(get_security_state)"
	if [ "$sstate" != "locked" ]; then
		echo "Incorrect security state: $sstate expected: locked"
		err "$LINENO"
	fi
}

get_security_state()
{
	$NDCTL list -i -b "$NFIT_TEST_BUS0" -d "$dev" | jq -r .[].dimms[0].security
}

setup_passphrase()
{
	$NDCTL setup-passphrase "$dev" -k user:"$masterkey"
	sstate="$(get_security_state)"
	if [ "$sstate" != "unlocked" ]; then
		echo "Incorrect security state: $sstate expected: unlocked"
		err "$LINENO"
	fi
}

remove_passphrase()
{
	$NDCTL remove-passphrase "$dev"
	sstate="$(get_security_state)"
	if [ "$sstate" != "disabled" ]; then
		echo "Incorrect security state: $sstate expected: disabled"
		err "$LINENO"
	fi
}

erase_security()
{
	$NDCTL sanitize-dimm -c "$dev"
	sstate="$(get_security_state)"
	if [ "$sstate" != "disabled" ]; then
		echo "Incorrect security state: $sstate expected: disabled"
		err "$LINENO"
	fi
}

update_security()
{
	$NDCTL update-passphrase "$dev"
	sstate="$(get_security_state)"
	if [ "$sstate" != "unlocked" ]; then
		echo "Incorrect security state: $sstate expected: unlocked"
		err "$LINENO"
	fi
}

freeze_security()
{
	$NDCTL freeze-security "$dev"
}

test_1_security_setup_and_remove()
{
	setup_passphrase
	remove_passphrase
}

test_2_security_setup_and_update()
{
	setup_passphrase
	update_security
	remove_passphrase
}

test_3_security_setup_and_erase()
{
	setup_passphrase
	erase_security
}

test_4_security_unlock()
{
	setup_passphrase
	lock_dimm
	$NDCTL enable-dimm "$dev"
	sstate="$(get_security_state)"
	if [ "$sstate" != "unlocked" ]; then
		echo "Incorrect security state: $sstate expected: unlocked"
		err "$LINENO"
	fi
	$NDCTL disable-region -b "$NFIT_TEST_BUS0" all
	remove_passphrase
}

# this should always be the last test. with security frozen, nfit_test must
# be removed and is no longer usable
test_5_security_freeze()
{
	setup_passphrase
	freeze_security
	sstate="$(get_security_state)"
	if [ "$sstate" != "frozen" ]; then
		echo "Incorrect security state: $sstate expected: frozen"
		err "$LINENO"
	fi
	$NDCTL remove-passphrase "$dev" && { echo "remove succeed after frozen"; }
	sstate="$(get_security_state)"
	echo "$sstate"
	if [ "$sstate" != "frozen" ]; then
		echo "Incorrect security state: $sstate expected: frozen"
		err "$LINENO"
	fi
}

check_min_kver "5.0" || do_skip "may lack security handling"
uid="$(keyctl show | grep -Eo "_uid.[0-9]+" | head -1 | cut -d. -f2-)"
if [ "$uid" -ne 0 ]; then
	do_skip "run as root or with a sudo login shell for test to work"
fi

modprobe nfit_test
setup
check_prereq "keyctl"
rc=1
detect
test_cleanup
setup_keys
echo "Test 1, security setup and remove"
test_1_security_setup_and_remove
echo "Test 2, security setup, update, and remove"
test_2_security_setup_and_update
echo "Test 3, security setup and erase"
test_3_security_setup_and_erase
echo "Test 4, unlock dimm"
test_4_security_unlock

# Freeze should always be run last because it locks security state and require
# nfit_test module unload.
echo "Test 5, freeze security"
test_5_security_freeze

test_cleanup
_cleanup
exit 0
