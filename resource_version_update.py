#!/usr/bin/env python3
# Resource file version information updater
# Sets the product and file version of the resource to match `version.h`
# Copyright 2025 Persune, MIT

# parse version info in version.h
def find_line_containing(string: str, find_second = False):
    with open("version.h", "r") as version_h:
        checked_first_line = False
        for line in version_h:
            if string in line:
                if not checked_first_line and find_second:
                    checked_first_line = True
                    continue
                else: return line

def get_version_define_value(string: str, find_second = False):
    # first define is:
    # `#define VERSION_xxx <int>`
    # we grab the last character of the line for the version number
    return int(find_line_containing(string, find_second).rsplit(string)[1])

ver_api = get_version_define_value("VERSION_API")
ver_maj = get_version_define_value("VERSION_MAJ")
ver_min = get_version_define_value("VERSION_MIN")

# build type is tricky; we need to check if `WIP` is "defined"
# this means if the line is commented out with `//` or not
release = find_line_containing("WIP").startswith("//")
ver_bld = get_version_define_value("VERSION_BLD", release)

import fileinput
import sys

# modify Dn-FamiTracker.rc
def overwrite_rc(key_str: str, val_str: str):
    for line in fileinput.input(files="Dn-FamiTracker.rc", mode="r", inplace=True):
        if key_str in line:
            line = line.rsplit(key_str)[0] + key_str + val_str + "\n"
        sys.stdout.write(line)

overwrite_rc(
    "FILEVERSION",
    f" {ver_api},{ver_maj},{ver_min},{ver_bld}"
)
overwrite_rc(
    "PRODUCTVERSION",
    f" {ver_api},{ver_maj},{ver_min},{ver_bld}"
)
overwrite_rc(
    "VALUE \"ProductVersion\"",
    f", \"{ver_api}.{ver_maj}.{ver_min}\""
)
overwrite_rc(
    "VALUE \"FileVersion\"",
    f", \"{ver_api}.{ver_maj}.{ver_min}.{ver_bld}\""
)


print(f"Resource versions updated as {ver_api}.{ver_maj}.{ver_min}.{ver_bld}")