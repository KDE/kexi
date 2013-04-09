#!/usr/bin/python -Qwarnall

#
# This script fixes trivial errors in the sources:
#  - Add a newline to files that don't end in one. (only apply to source files!)
#  - Normalize SIGNAL and SLOT signatures
#  - Improve #include status: remove duplicates
#  - ...more later
#

import sys
import os
import string
import getopt
import re
import fnmatch


# Global variables
dryrun = False    # Maybe the default should be True...
pattern = "*"
recursive = False
verbose = False


# ----------------------------------------------------------------
#                         Individual actions


def doIncludes(contents):
    global verbose
    global dryrun

    # Compile a regex that matches an include statement.
    # We do no syntax check so    #include "foo> is found by this.
    includePattern = '^(\\#include\\s*)(["<])([^">]+)([">])(.*)'
    includeRegex = re.compile(includePattern)

    lineNo = 1
    newContents = []
    foundIncludes = []
    for line in contents:
        #print "input: ", line

        # See if this line is an include line.  If not, just append it
        # to the output and continue.
        includes = includeRegex.findall(line)
        #print "includes: ", includes
        if len(includes) == 0:
            newContents.append(line)
            lineNo = lineNo + 1
            continue

        include = includes[0]
        # Here we know it's an include statement.  We should only have found one hit.
        #  include[0] = "#include "
        #  include[1] = "\"" or "<"
        #  include[2] = the filename in the include statement
        #  include[3] = "\"" or ">"
        #  include[4] = the rest of the line
        includeName = include[2]
        #print "include name: ", includeName
        
        if includeName in foundIncludes:
            if verbose:
                sys.stderr.write("  Duplicate include on line " + str(lineNo) + " (removing)\n")
        else:
            foundIncludes.append(includeName)
            newContents.append(line)

        lineNo = lineNo + 1

    return newContents


def doNormalize(contents):
    global verbose

    # Compile a regex that matches SIGNAL or SLOT followed by two
    # parenthesis levels (signal/slot + function)
    #
    # Warning: This is a very simple parser that will fail on multi-line
    #          cases or cases with more than 2 parenthesis.  But it should
    #          cover the majority of cases.
    topPattern = "(SIGNAL|SLOT)(\\s*\\([^\\(]*\\([^\\)]*\\)\s*\\))"
    topRegex = re.compile(topPattern)

    # regex to identify the parameter list
    #   [0]: Start parenthesis
    #   [1]: the identifier (function name of the signal or slot)
    #   [2]: the parameter list
    #   [3]: end parenthesis
    # Note: this won't work on function parameters with their own parameter list.
    paramPattern = "(\s*\\(\s*)([a-zA-z][a-zA-z0-9]*)(\s*\\(\s*[^\\)]*\s*\\))(\s*\\)\s*)"
    paramRegex = re.compile(paramPattern)

    lineNo = 1
    newContents = []
    for line in contents:
        #print "input: ", line

        # replace signal/slot signatures with the normalized equivalent
        signalSlotMacros = topRegex.findall(line)
        for macro in signalSlotMacros:
            # Parameters to SIGNAL or SLOT
            signalSlotParam = macro[1] #macro[0] = "SIGNAL" or "SLOT"

            params = paramRegex.findall(signalSlotParam)
            if not params:
                continue
            params = params[0]  # There should only be one hit
            #print "params:", params, len(params)

            functionName  = params[1]
            paramList = params[2].strip()[1:-1]  # remove the parenthesis
            #print repr(functionName), repr(paramList)

            # Now we have dug out the parameter list to the function
            # This sequence does the actual normalizing.
            functionParams = paramList.split(",")
            #print functionParams
            outParamList = []
            for functionParam in functionParams:
                s = functionParam.strip()
                if s[:5] == "const" and s[-1:] == "&":
                    # Remove const-&
                    outParamList.append(s[5:-1].replace(" ", ""))
                elif s[:5] == "const":
                    # Remove const without &
                    outParamList.append(s[5:].replace(" ", ""))
                elif s[-1:] == "&":
                    # Keep & without const but remove spaces
                    outParamList.append(s.replace(" ", ""))
                else:
                    # Keep parameter without any const and & but remove spaces
                    # (This and the last line could be combined.)
                    outParamList.append(s.replace(" ", ""))
            outParams = string.join(outParamList, ",")
            signalSlotParam2 = "(" + functionName + "(" + outParams + "))"

            if verbose and signalSlotParam != signalSlotParam2:
                sys.stderr.write("  Normalizing " + macro[0] + " statement on line "
                                 + str(lineNo) + "\n")

            line = line.replace(signalSlotParam, signalSlotParam2, 1)
            #print "result: ", line

        # When we exit the loop all the substitutions inside the line are done.
        newContents.append(line)
        lineNo = lineNo + 1

    return newContents


# ----------------------------------------------------------------


def handleFile(name, actions):
    global dryrun, verbose

    if verbose:
        sys.stderr.write(name + ":\n")

    # Read the contents of the file into a list, one item per line.
    infile = open(name)
    contents = infile.readlines()
    infile.close()

    if "endswitheol" in actions:
        if contents != [] and contents[-1] != "" and contents[-1][-1] != "\n":
            if verbose:
                sys.stderr.write("  Adding EOL to end of file\n")
            contents[-1] = contents[-1] + "\n"
    if "normalize" in actions:
        contents = doNormalize(contents)
    if "includes" in actions:
        contents = doIncludes(contents)

    if not dryrun:
        outname = name + "--temp"  #FIXME: Generate real tempname
        outfile = open(outname, "w")
        outfile.writelines(contents)
        outfile.close()
        os.rename(outname, name)


def traverseTree(dir, actions, names):
    global recursive, verbose, pattern

    # We could also use os.walk()
    for name in names:
        if dir == "":
            fullname = name
        else:
            fullname = dir + "/" + name

        if not os.path.exists(fullname):
            sys.stderr.write(fullname + ": unknown file or directory\n")
            continue

        if os.path.isdir(fullname):
            if recursive:
                traverseTree(fullname, actions, os.listdir(fullname))
            # Ignore all directories if not in recursive mode
        else:
            if fnmatch.fnmatch(name, pattern):
                handleFile(fullname, actions)


# ================================================================


def usage(errormsg=""):
    if errormsg:
        print "Error:", sys.argv[0] + ":", errormsg, "\n"
    else:
        print "Fix trivial errors in the source tree.\n"

    print """usage: fixsrc.py [options] [files]
    options:
        -a --actions    a comma separated list of the following possible actions:
                          endswitheol:  adds newline at the end to files that don't end in newline
                          normalize:    normalizes SIGNAL and SLOT signatures
                          includes:     improves #include status (currently: removes duplicates)
                          all:          all of the above
        -d --dryrun     don't actually perform the actions (combine with --verbose)
                        This is recommended before doing the full run.
        -h --help       print this help and exit immediately
        -p --pattern    apply to files whose name matches a certain glob pattern
        -r --recursive  recursive: all files that are directories are traversed recursively
        -v --verbose    print extra verbose output

    files:
        source files to be fixed and/or directories if --recursive is given

    examples:
        fixsrc.py -rv --actions endswitheol libs
        fixsrc.py -rv --pattern '*.cpp' --actions normalize .
"""
    sys.exit(0)


def main():
    global dryrun, recursive, pattern, verbose
    allActions = ["endswitheol", "normalize", "includes", "all"]

    try :
        opts, params = getopt.getopt(sys.argv[1:], "a:dhp:rv" ,
                                     ["actions=", "dryrun", "help", "pattern=", "recursive", "verbose"])
    except getopt.GetoptError:
        usage("unknown options")
    #print "opts:", opts
    #print "params:", params

    actions = []
    for opt, param in opts:
        #print opt, param
        if opt in ("-a" , "--actions"):
            actions = string.split(param, ",")
            #print "actions: ", actions
            for a in actions:
                if not a in allActions:
                    usage("unknown action: " + a + "\n\n")
            if "all" in actions:
                actions = allActions[:-1] # Remove "all", which is a meta action.
                
        elif opt in ("-d" , "--dryrun"):
            dryrun = True
        elif opt in ("-h" , "--help"):
            usage()
        elif opt in ("-p" , "--pattern"):
            pattern = param
        elif opt in ("-r" , "--recursive"):
            recursive = True
        elif opt in ("-v" , "--verbose"):
            verbose = True

    if actions == []:
        usage("no actions defined")

    # Do the actual work
    traverseTree("", actions, params)

    return 0

if __name__ == '__main__':
    main()
