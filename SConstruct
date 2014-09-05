#-*- mode: python; -*-
#build file

import datetime
import os
import re
import shutil
import stat
import sys
import types
import time

#options
#option functions
#
options = {}
options_topass = {}
def add_option(name, help, nargs, constributesToVariantDir,
               dest=None, default = None, type="string", choices=None):
    if dest is None:
        dest = name

    AddOption("--" + name,
              dest = dest,
              nargs = nargs,
              action = "store",
              choices = choices,
              default = default,
              help = help)
    options[name] = {"help":help,
                     "nargs":nargs,
                     "constributesToVariantDir":constributesToVariantDir,
                     "dest":dest,
                     "default":default}

def get_option(name):
    return GetOption(name)

def _has_option(name):
    x = get_option(nam)
    if x is None:
        return False
    if x == False:
        return False
    return True

def has_option(name):
    x = _has_option(name)
    if name not in options_topass:
        options_topass[name] = x
    return x

add_option("64", "whether to force 64 bit", 0, True, "force64");
add_option("32", "whether to force 32 bit", 0, True, "force32");
add_option("use-google-tcmalloc", "Link google-tcmalloc library", 0, True)
add_option("use-google-profiler", "Link google-perftool library", 0, False)
add_option("use-google-lint", "Use google-lint script", 0, False)

SetOption("implicit_cache", 1)

env = Environment()
env.Append(CPPPATH=".")
print env['CPPPATH']
# Export('env')
# env.SConscript('SConscript')
env.Program('base_test.cc')
