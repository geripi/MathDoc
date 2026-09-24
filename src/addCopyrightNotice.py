#! /bin/python3

import os

def add_copyright_header(filepath):
    """
    Opens a file, checks if the first line is empty. If it is, replaces it
    with the specified copyright text. If not, inserts the copyright text
    at the very beginning of the file, followed by a newline.
    Returns True if the file was modified, False otherwise (e.g., error).
    """
    copyright_text = """/*
Copyright (C) 2025 Gerald Pichler

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see https://www.gnu.org/licenses/.
*/
"""
    try:
        # Read the entire content of the file
        with open(filepath, 'r', encoding='utf-8') as f:
            lines = f.readlines()

        modified = False
        new_content_lines = []

        if not lines:
            # If the file is empty, just add the copyright text
            new_content_lines.append(copyright_text)
            modified = True
        elif lines[0].strip() == "":
            # If the first line is empty, replace it with the copyright text
            new_content_lines.append(copyright_text)
            new_content_lines.extend(lines[1:])
            modified = True
        else:
            # If the first line is not empty, add the copyright text
            # followed by a newline, then the original content
            new_content_lines.append(copyright_text)
            new_content_lines.extend(lines)
            modified = True

        if modified:
            # Write the modified content back to the file
            with open(filepath, 'w', encoding='utf-8') as f:
                f.writelines(new_content_lines)
            return True
        else:
            return False # Should not happen with current logic, but good for robustness

    except FileNotFoundError:
        print(f"Error: File not found at '{filepath}'.")
        return False
    except Exception as e:
        print(f"An unexpected error occurred while processing '{filepath}': {e}")
        return False

def process_files_in_directory(directory="."):
    """
    Walks through the specified directory and its subdirectories,
    identifying and processing all .h and .cpp files to add the copyright header.
    """
    print(f"Starting to search for .h and .cpp files in '{os.path.abspath(directory)}'...")
    files_scanned = 0
    files_modified = 0

    # os.walk generates the file names in a directory tree by walking the tree
    for root, _, files in os.walk(directory):
        for filename in files:
            # Check if the file has a .h or .cpp extension
            if filename.endswith(".h") or filename.endswith(".cpp"):
                filepath = os.path.join(root, filename)
                files_scanned += 1
                print(f"Checking: {filepath}")

                # Attempt to add the copyright header
                if add_copyright_header(filepath):
                    files_modified += 1
                    print(f"  Successfully added copyright header to: {filepath}")
                else:
                    print(f"  No changes made to: {filepath} (or an error occurred)")

    print(f"\n--- Processing Summary ---")
    print(f"Total .h and .cpp files scanned: {files_scanned}")
    print(f"Number of files modified: {files_modified}")
    if files_modified > 0:
        print("IMPORTANT: Files have been modified in place. Please verify the changes and ensure your codebase is intact.")
    else:
        print("No files were modified, either because no files were found or errors occurred.")

# This block ensures the main function is called when the script is executed
if __name__ == "__main__":
    # You can change '.' to a specific directory path, e.g., 'C:/my_project' or '/home/user/code'
    # If you leave it as '.', it will process files in the directory where the script is run
    process_files_in_directory(".")
