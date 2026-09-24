#! /bin/python3

import os

def remove_first_multi_line_comment(filepath):
    """
    Opens a file, checks for the first '/*' and '*/' block.
    If "Copyright (C)" is found within this block (inclusive of '/*' and '*/'),
    the block is deleted, and the modified content is saved back to the file.
    Returns True if the file was modified, False otherwise.
    """
    try:
        # Read the entire content of the file
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()

        # Find the first occurrence of '/*'
        start_index = content.find("/*")
        if start_index == -1:
            # If '/*' is not found, there's nothing to remove
            return False

        # Find the first occurrence of '*/' AFTER the '/*'
        end_index = content.find("*/", start_index + 2)
        if end_index == -1:
            # If '/*' is found but no matching '*/' after it, we print a warning and skip
            print(f"Warning: Found '/*' in '{filepath}' but no matching '*/' after it. Skipping this file.")
            return False

        # Extract the content of the potential comment block, including '/*' and '*/'
        comment_block_content = content[start_index : end_index + 2]

        # Check if "Copyright (C)" is present within this specific comment block
        if "Copyright (C)" in comment_block_content:
            # Construct the new content:
            # - Take everything before the '/*'
            # - Concatenate with everything after the '*/' (skipping '*/' itself, which is 2 characters long)
            modified_content = content[:start_index] + content[end_index + 2:]

            # Write the modified content back to the file, overwriting the original
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(modified_content)

            return True  # Indicate that the file was modified
        else:
            # "Copyright (C)" was not found in the comment block, so do not modify
            return False

    except FileNotFoundError:
        print(f"Error: File not found at '{filepath}'.")
        return False
    except Exception as e:
        print(f"An unexpected error occurred while processing '{filepath}': {e}")
        return False

def process_files_in_directory(directory="."):
    """
    Walks through the specified directory and its subdirectories,
    identifying and processing all .h and .cpp files.
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

                # Attempt to remove the comment block conditionally
                if remove_first_multi_line_comment(filepath):
                    files_modified += 1
                    print(f"  Successfully modified: {filepath}")
                else:
                    print(f"  No changes made to: {filepath} (either no comment, or 'Copyright (C)' not found)")

    print(f"\n--- Processing Summary ---")
    print(f"Total .h and .cpp files scanned: {files_scanned}")
    print(f"Number of files modified: {files_modified}")
    if files_modified > 0:
        print("IMPORTANT: Files have been modified in place. Please verify the changes and ensure your codebase is intact.")
    else:
        print("No files were modified, either because no matching comments were found, 'Copyright (C)' was not found within them, or errors occurred.")

# This block ensures the main function is called when the script is executed
if __name__ == "__main__":
    # You can change '.' to a specific directory path, e.g., 'C:/my_project' or '/home/user/code'
    # If you leave it as '.', it will process files in the directory where the script is run
    process_files_in_directory(".")

