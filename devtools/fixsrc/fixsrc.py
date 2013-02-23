#!/usr/bin/python -Qwarnall

#
# This script fixes trivial errors in the sources:
#  - Add a newline to files that don't end in one. (only apply to source files!)
#  - Normalize SIGNAL and SLOT signatures
#  - ...more later
#

import sys
import os
import string
import getopt
import re


# Global variables
dryrun = False    # Maybe the default should be True...
recursive = False
verbose = False


# ----------------------------------------------------------------
#                         Individual actions


def doNormalize(contents):
    global verbose

    # Compile a regex that matches SIGNAL or SLOT followed by two
    # parenthesis levels (signal/slot + function)
    #
    # Warning: This is a very simple parser that will fail on multi-line
    #          cases or cases with more than 2 parenthesis.  But it should
    #          cover the majority of cases.
    pattern = "(SIGNAL|SLOT)(\\s*\\([^\\(]*\\([^\\)]*\\)\s*\\))"
    regex = re.compile(pattern)

    lineNo = 1
    newContents = []
    for line in contents:
        #print line

        # replace signal/slot signatures with the normalized equivalent
        matches = regex.findall(line)
        #print "matches:", matches
        for match in matches:
            # Parameters to SIGNAL or SLOT
            param = match[1] #match[0] = SIGNAL or SLOT
            param2 = param.replace("const", "").replace("&", "").replace(" ", "")

            if verbose and param != param2:
                sys.stderr.write("  Normalizing " + match[0] + " statement on line "
                                 + str(lineNo) + "\n")

            line = line.replace(param, param2, 1)

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

    if not dryrun:
        outname = name + "--temp"  #FIXME: Generate real tempname
        outfile = open(outname, "w")
        outfile.writelines(contents)
        outfile.close()
        os.rename(outname, name)


def traverseTree(dir, actions, names):
    global recursive, verbose

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
                          all:          all of the above
        -d --dryrun     don't actually perform the actions (combine with --verbose)
                        This is recommended before doing the full run.
        -h --help       print this help and exit immediately
        -r --recursive  recursive: all files that are directories are traversed recursively
        -v --verbose    print extra verbose output

    files:
        source files to be fixed and/or directories if --recursive is given

    example:
        fixsrc.py -rv --actions normalize libs
"""
    sys.exit(0)


def main():
    global dryrun, recursive, verbose
    allActions = ["endswitheol", "normalize", "all"]

    try :
        opts, params = getopt.getopt(sys.argv[1:], "a:dhrv" ,
                                     ["actions=", "dryrun", "help", "recursive", "verbose"])
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
