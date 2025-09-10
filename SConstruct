#!/usr/bin/env python
import os
import sys

from methods import print_error, print_special

def check_submodule(dir_name):
    if os.path.isdir(dir_name):
        if os.listdir(dir_name):
            return

    err_msg = str.format("""{} is not available within this folder, as Git submodules haven't been initialized.
    Run the following command to download {}:

         git submodule update --init --recursive""", dir_name)
    sys.exit(1)


libname = "hydrogen.gd.behaviors"
projectdir = "demo"

localEnv = Environment(tools=["default"], PLATFORM="")

customs = ["custom.py"]
customs = [os.path.abspath(path) for path in customs]

opts = Variables(customs, ARGUMENTS)

opts.Add(BoolVariable("tests", "Build tests", False))

opts.Update(localEnv)
Help(opts.GenerateHelpText(localEnv))

env = localEnv.Clone()

check_submodule('godot-cpp')
env = SConscript("godot-cpp/SConstruct", {"env": env, "customs": customs})

env.Append(CPPPATH=["src/"])
sources = Glob("src/*.cpp")
sources.append(Glob("src/pipelines/*.cpp"))
sources.append(Glob("src/behavior_trees/*.cpp"))

env.tests = env["tests"]

if env.tests:
    check_submodule('doctest')

    env.Append(CPPDEFINES=["TESTS_ENABLED"])

    # We must disable the THREAD_LOCAL entirely in doctest to prevent crashes on debugging
    # Since we link with /MT thread_local is always expired when the header is used
    # So the debugger crashes the engine and it causes weird errors
    # Explained in https://github.com/onqtam/doctest/issues/401
    if env["platform"] == "windows":
        env.Append(CPPDEFINES=[("DOCTEST_THREAD_LOCAL", "")])

    if env["disable_exceptions"]:
        env.Append(CPPDEFINES=["DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS"])

    env.Append(CPPPATH=[
        "doctest/doctest/",
        "tests/"])
    sources += Glob("tests/*.cpp")

    print_special("Included tests")

if env["target"] in ["editor", "template_debug"]:
    try:
        doc_data = env.GodotCPPDocData("src/gen/doc_data.gen.cpp", source=Glob("doc_classes/*.xml"))
        sources.append(doc_data)
    except AttributeError:
        print("Not including class reference as we're targeting a pre-4.3 baseline.")

file = "{}{}{}".format(libname, env["suffix"], env["SHLIBSUFFIX"])
filepath = ""

if env["platform"] == "macos" or env["platform"] == "ios":
    filepath = "{}.framework/".format(env["platform"])
    file = "{}{}".format(libname, env["suffix"])

libraryfile = "bin/{}/{}{}".format(env["platform"], filepath, file)
library = env.SharedLibrary(
    libraryfile,
    source=sources,
)

file_target = "{}/bin/{}/{}lib{}".format(projectdir, env["platform"], filepath, file)

copy = env.InstallAs(file_target, library)

default_args = [library, copy]

dev_index = file_target.find(".dev")
if dev_index > -1:
    print_special("copying dev lib")
    dev_target = file_target.replace(".dev", "")
    dev_copy = env.InstallAs(dev_target, library)
    default_args.append(dev_copy)

Default(*default_args)
