#!/usr/bin/env python3
import os
import sys
import re

def preprocess_proto(src_path, dest_path, package_name):
    """
    Reads a .proto file, adds a package statement if missing,
    and updates absolute type references (e.g. .MessageName -> .package.MessageName).
    Does not touch string literals (like imports) or comments.
    """
    with open(src_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # 1. Add the package statement if not already present
    if not re.search(r'^\s*package\s+[a-zA-Z0-9_.]+;', content, re.MULTILINE):
        # Look for a syntax declaration line (e.g., syntax = "proto2";)
        syntax_match = re.search(r'^\s*syntax\s*=\s*["\']proto[23]["\'];', content, re.MULTILINE)
        package_line = f"\npackage {package_name};\n"
        if syntax_match:
            # Place the package statement right after the syntax statement
            insert_idx = syntax_match.end()
            content = content[:insert_idx] + package_line + content[insert_idx:]
        else:
            # If no syntax statement, prepend to the very top
            content = package_line + content

    # 2. Update fully qualified absolute references (starting with a dot)
    # We use a pattern that matches comments, string literals, and type references,
    # but only modifies the type references.
    pattern = re.compile(
        r'(?P<comment>//.*|/\*[\s\S]*?\*/)'                  # Comments (single and multi-line)
        r'|(?P<string>"(?:[^"\\]|\\.)*"|\'(?:[^\'\\]|\\.)*\')' # String literals (double and single quoted)
        r'|(?P<skip>^\s*package\s+[a-zA-Z0-9_.]+;|^\s*import\s+[^;]+;)' # Package and import statements
        r'|(?P<type>\.([a-zA-Z_][a-zA-Z0-9_.]*))',            # Fully qualified type references
        re.MULTILINE
    )

    def replace_match(match):
        # If it matches a comment, string, or skip pattern, return it unchanged
        if match.group('comment') or match.group('string') or match.group('skip'):
            return match.group(0)
        
        type_str = match.group('type')
        type_path = type_str[1:]
        
        # Keep external packages (like google's built-in descriptors) intact
        if type_path.startswith("google."):
            return type_str
        
        return f".{package_name}.{type_path}"

    content = pattern.sub(replace_match, content)

    # Ensure destination directory exists and write
    os.makedirs(os.path.dirname(dest_path), exist_ok=True)
    with open(dest_path, 'w', encoding='utf-8') as f:
        f.write(content)

def main():
    if len(sys.argv) < 4:
        print("Usage:")
        print("  python preprocess_protos.py <src_path> <dest_path> <package_name>")
        sys.exit(1)
        
    src_path = sys.argv[1]
    dest_path = sys.argv[2]
    package_name = sys.argv[3]

    if os.path.isdir(src_path):
        # Process a directory recursively
        for root, _, files in os.walk(src_path):
            for file in files:
                if file.endswith('.proto'):
                    src_file = os.path.join(root, file)
                    rel_path = os.path.relpath(src_file, src_path)
                    dest_file = os.path.join(dest_path, rel_path)
                    preprocess_proto(src_file, dest_file, package_name)
    else:
        # Process a single file
        preprocess_proto(src_path, dest_path, package_name)

if __name__ == '__main__':
    main()
