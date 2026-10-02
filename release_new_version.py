#!/usr/bin/env python

import re
import subprocess
import sys
from pathlib import Path


REPO = Path("/home/gerald/Software_Programmieren_Privat/MathDoc")
MAIN_CPP = REPO / "src" / "main.cpp"


def run(command):
    print(f"$ {' '.join(command)}")
    result = subprocess.run(command)

    if result.returncode != 0:
        sys.exit(result.returncode)


# Need at least one argument for the commit message
if len(sys.argv) < 2:
    print(f"Usage: {sys.argv[0]} COMMIT_MESSAGE [MORE WORDS ...]")
    sys.exit(1)


# Everything except argv[0] becomes the commit message
commit_message = " ".join(sys.argv[1:])


# Read src/main.cpp
try:
    content = MAIN_CPP.read_text()
except OSError as e:
    print(f"Could not read {MAIN_CPP}: {e}")
    sys.exit(1)


# Find:
# app.setApplicationVersion("1.0.4");
pattern = r'app\.setApplicationVersion\("(\d+)\.(\d+)\.(\d+)"\);'

print("Searching for pattern '" + pattern + "' in main.cpp.\n")

match = re.search(pattern, content)

if not match:
    print(f"Could not find application version in {MAIN_CPP}")
    sys.exit(1)


# Extract and increment the patch version
major = int(match.group(1))
minor = int(match.group(2))
patch = int(match.group(3)) + 1

new_version = f"{major}.{minor}.{patch}"

# Replace the old version with the new version
new_line = f'app.setApplicationVersion("{new_version}");'

content = re.sub(pattern, new_line, content, count=1)


# Write the modified main.cpp
try:
    MAIN_CPP.write_text(content)
except OSError as e:
    print(f"Could not write {MAIN_CPP}: {e}")
    sys.exit(1)


print(f"Updated application version to {new_version}")


# Add the entire repository
run(["git", "add", "--", str(REPO)])

# Commit all changes
run([ "git", "-C", str(REPO), "commit", "-m", commit_message,])

# Create/update the tag
tag = f"v{new_version}"
run(["git", "-C", str(REPO), "tag", "-f", tag,])

# Push the current branch
run(["git", "-C", str(REPO), "push", "origin", "HEAD",])

# Force-push the tag
run(["git", "-C", str(REPO), "push", "-f", "origin", tag,])
