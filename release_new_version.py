#! /bin/python

import subprocess
import sys

main

REPO = "/home/gerald/Software_Programmieren_Privat/MathDoc"

run(["git", "-C", REPO, "add", "-A"])
run(["git", "-C", REPO, "commit", "-m", "Update changes"])
run(["git", "-C", REPO, "tag", "-f", "v1.0.2"])
run(["git", "-C", REPO, "push", "-f", "origin", "v1.0.2"])
