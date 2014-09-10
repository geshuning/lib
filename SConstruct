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

import SCons
from SCons.Script.SConscript import SConsEnvironment
import SCons.Action
import SCons.Builder

vars = Variables(None, ARGUMENTS)
vars.AddVariables(
    BoolVariable("shared", "Build with shared libraries", True),
    BoolVariable("strip", "Strip all installed binaries", True),
    BoolVariable("debug", "Build with debug messages", True),
    PathVariable("prefix", "Install prefix", '.', PathVariable.PathAccept),
    PathVariable("shared-lib-prefix", "Install prefix", 'shared_libs',
                 PathVariable.PathAccept),
    EnumVariable("profile", "Build with profiling", "no",
                 allowed_values=("no", "gprof"), map={}, ignorecase=2),

)
CURRENT_DIR = os.getcwd()

# CPPPATH
CPPPATH_common = CURRENT_DIR
CPPPATH_gtest  = "%s/%s" % (CURRENT_DIR, "unit_testing/gtest-1.7.0/include")
cpp_path = [
    CPPPATH_common,
    CPPPATH_gtest
]
# CPPFLAGS
cpp_flags = [
    '-g',
]
# CPPDEFINES
cpp_defines = {
    "RELEASE_BUILD" : "${RELEASE}",
}
# LIBPATH
LIBPATH_gtest = "%s/%s" % (CURRENT_DIR, "unit_testing/gtest-1.7.0/")
lib_path = [
    LIBPATH_gtest,
]
# LIBS
# Put them in dependent order
LIBS_common = "pthread"
libs = [
    "gtest",
    LIBS_common,
]

env = Environment()
# Variables
env.Append(variables = vars)
# print '${shared-lib-prefix}'
# Compile environment
env.Append(CPPPATH = cpp_path)
env.Append(CPPFLAGS = cpp_flags)
env.Append(CPPDEFINES = cpp_defines)
env.Append(LIBPATH = lib_path)
env.Append(LIBS = libs)
# Install environment
# env.Append(variables = vars, SHARED_LIB_PATH=env['shared-lib-prefix'])
shared_lib_path = "%s/%s" % (CURRENT_DIR, "shared_libs")
env.Append(SHARED_LIB_PATH=shared_lib_path)

# Export it
Export('env')
env.SConscript('SConscript')
env.Program("base_unit_test",
            ["base_test.cc",
             "base/memory/scoped_ptr_unittest.cc"])
# Create help message
env.Help(vars.GenerateHelpText(env))
