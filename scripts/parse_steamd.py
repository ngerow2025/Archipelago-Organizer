#!/usr/bin/env python3
import os
import sys
import re
import argparse

# Mappings of steamd types to C++ types
TYPE_MAP = {
    'uint': 'uint32_t',
    'byte': 'uint8_t',
    'int': 'int32_t',
    'ushort': 'uint16_t',
    'ulong': 'uint64_t',
    'long': 'int64_t',
    'short': 'int16_t',
    'sbyte': 'int8_t',
    'bool': 'bool',
    'SteamKit2.GC.Internal.CMsgProtoBufHeader': 'steam::proto::CMsgProtoBufHeader',
    'SteamKit2.Internal.CMsgProtoBufHeader': 'steam::proto::CMsgProtoBufHeader',
}

def rewrite_default_value(val_expr):
    if not val_expr:
        return val_expr
    # Replace C# MaxValue/MinValue constants with C++ equivalents
    val_expr = val_expr.replace('ulong.MaxValue', '0xFFFFFFFFFFFFFFFFULL')
    val_expr = val_expr.replace('uint.MaxValue', '0xFFFFFFFFUL')
    val_expr = val_expr.replace('ushort.MaxValue', '0xFFFFU')
    val_expr = val_expr.replace('byte.MaxValue', '0xFFU')
    return val_expr

def parse_steamd_file(file_path):
    imports = []
    enums = []
    classes = []
    
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        
    lines = content.splitlines()
    in_enum = False
    in_class = False
    current_enum = None
    current_class = None
    
    # Regex to match: enum EName[<type>] [flags]
    # or public enum EName[<type>] [flags]
    enum_header_re = re.compile(r"^\s*(?:public\s+)?enum\s+(\w+)(?:<([^>]+)>)?(?:\s+(\w+))?")
    # Regex to match: class ClassName[<GenericArg>] [removed]
    class_header_re = re.compile(r"^\s*(?:public\s+)?class\s+(\w+)(?:<(\w+)::(\w+)>)?(?:\s+(removed))?")
    # Regex to match import
    import_re = re.compile(r'^\s*#import\s+"([^"]+)"')
    
    for line_num, line in enumerate(lines, 1):
        stripped = line.strip()
        
        if not in_enum and not in_class:
            if not stripped or stripped.startswith('//'):
                continue
            
            # Check for import
            import_match = import_re.match(stripped)
            if import_match:
                imports.append(import_match.group(1))
                continue
                
            if stripped.startswith('#'):
                # Ignore other preprocessor-like directives or comments starting with #
                continue
                
            # Check for enum header
            match = enum_header_re.match(stripped)
            if match:
                name = match.group(1)
                base_type = match.group(2)
                flags = match.group(3)
                
                current_enum = {
                    'name': name,
                    'base_type': base_type,
                    'is_flags': flags == 'flags',
                    'values': []
                }
                in_enum = True
                continue
                
            # Check for class header
            match = class_header_re.match(stripped)
            if match:
                name = match.group(1)
                msg_type_enum = match.group(2)
                msg_type_val = match.group(3)
                is_removed = match.group(4) == 'removed'
                
                current_class = {
                    'name': name,
                    'msg_type_enum': msg_type_enum,
                    'msg_type_val': msg_type_val,
                    'is_removed': is_removed,
                    'fields': []
                }
                in_class = True
                continue
                
            continue
            
        if in_enum:
            # We are in_enum. Look for closing brace.
            if stripped.startswith('}') or stripped.startswith('};'):
                enums.append(current_enum)
                current_enum = None
                in_enum = False
                continue
                
            if stripped == '{':
                continue
                
            # Parse enum values
            if not stripped:
                continue
                
            if stripped.startswith('//'):
                current_enum['values'].append({
                    'type': 'comment',
                    'text': stripped
                })
                continue
                
            if '=' not in stripped:
                continue
                
            # Split by first semicolon
            parts = stripped.split(';', 1)
            assignment = parts[0].strip()
            rest = parts[1].strip() if len(parts) > 1 else ""
            
            if '=' not in assignment:
                continue
                
            name_part, val_part = assignment.split('=', 1)
            val_name = name_part.strip()
            val_expr = val_part.strip()
            
            annotation = None
            comment = None
            
            # Check for inline comment in rest
            if '//' in rest:
                rest_part, comment_part = rest.split('//', 1)
                comment = comment_part.strip()
                rest = rest_part.strip()
            else:
                rest = rest.strip()
                
            if rest:
                annotation = rest
                
            if annotation and re.search(r'\bremoved\b', annotation, re.IGNORECASE):
                continue
                
            current_enum['values'].append({
                'type': 'value',
                'name': val_name,
                'value': val_expr,
                'annotation': annotation,
                'comment': comment
            })
            continue
            
        if in_class:
            # We are in_class. Look for closing brace.
            if stripped.startswith('}') or stripped.startswith('};'):
                if not current_class['is_removed']:
                    classes.append(current_class)
                current_class = None
                in_class = False
                continue
                
            if stripped == '{':
                continue
                
            # Parse class fields/constants
            if not stripped:
                continue
                
            if stripped.startswith('//'):
                current_class['fields'].append({
                    'type': 'comment',
                    'text': stripped
                })
                continue
                
            # Strip trailing semicolon if present, but keep comments
            comment = None
            if '//' in stripped:
                stripped, comment_part = stripped.split('//', 1)
                comment = comment_part.strip()
            
            stripped = stripped.strip()
            if stripped.endswith(';'):
                stripped = stripped[:-1].strip()
                
            if not stripped:
                continue
                
            # Parse field/constant
            # Format: [modifiers...] type name [= default]
            val_expr = None
            if '=' in stripped:
                stripped, val_expr = stripped.split('=', 1)
                stripped = stripped.strip()
                val_expr = val_expr.strip()
                
            tokens = stripped.split()
            if len(tokens) < 2:
                # Not a valid field
                continue
                
            name = tokens[-1]
            type_raw = tokens[-2]
            modifiers = tokens[:-2]
            
            # If 'const' is in modifiers, it's a class constant!
            is_const = 'const' in modifiers
            
            # Resolve C++ type
            array_match = re.match(r'^(\w+)<(\d+)>$', type_raw)
            if array_match:
                element_type_raw = array_match.group(1)
                array_size = int(array_match.group(2))
                element_type = TYPE_MAP.get(element_type_raw, element_type_raw)
                cpp_type = f"std::array<{element_type}, {array_size}>"
            else:
                cpp_type = TYPE_MAP.get(type_raw, type_raw)
                if '.' in cpp_type:
                    cpp_type = cpp_type.split('.')[-1]
                    
            rewritten_val = rewrite_default_value(val_expr)
            
            if is_const:
                current_class['fields'].append({
                    'type': 'const',
                    'name': name,
                    'cpp_type': cpp_type,
                    'value': rewritten_val,
                    'comment': comment
                })
            else:
                current_class['fields'].append({
                    'type': 'field',
                    'name': name,
                    'cpp_type': cpp_type,
                    'value': rewritten_val,
                    'comment': comment
                })
                
    return imports, enums, classes

def rewrite_expression(expr, enum_name, known_values, underlying_type):
    # Replaces terms in the expression that match known enum value names with the C++ qualified cast.
    def replace_word(match):
        word = match.group(1)
        if word in known_values:
            return f"(({underlying_type}){enum_name}::{word})"
        return word
    
    # We match identifiers (words)
    return re.sub(r'\b([a-zA-Z_]\w*)\b', replace_word, expr)

def generate_cpp_header(imports, enums, classes, namespace_name):
    output = []
    output.append("#pragma once")
    output.append("")
    output.append("#include <cstdint>")
    
    # Check if we need `<array>`
    has_array = False
    for cls in classes:
        for field in cls['fields']:
            if field['type'] in ('field', 'const') and 'std::array' in field['cpp_type']:
                has_array = True
                break
    if has_array:
        output.append("#include <array>")
        
    # Check if we need protobuf header
    has_proto = False
    for cls in classes:
        for field in cls['fields']:
            if field['type'] in ('field', 'const') and 'CMsgProtoBufHeader' in field['cpp_type']:
                has_proto = True
                break
    if has_proto:
        output.append('#include "steammessages_base.pb.h"')
        
    # Process imports
    if imports:
        for imp in sorted(list(set(imports))):
            imp_clean = imp.strip('"\'')
            if imp_clean.endswith('.steamd'):
                imp_h = imp_clean[:-7] + '.h'
            else:
                imp_h = imp_clean + '.h'
            output.append(f'#include "{imp_h}"')
            
    output.append("")
    output.append(f"namespace {namespace_name} {{")
    output.append("")
    
    # Output enums
    for i, enum in enumerate(enums):
        name = enum['name']
        base_type_raw = enum['base_type']
        is_flags = enum['is_flags']
        
        # Determine C++ underlying type
        if base_type_raw in TYPE_MAP:
            underlying_type = TYPE_MAP[base_type_raw]
        else:
            underlying_type = 'uint32_t' if is_flags else 'int32_t'
            
        output.append(f"enum class {name} : {underlying_type} {{")
        
        # Get list of known value names for rewriting expressions
        known_values = set()
        for item in enum['values']:
            if item['type'] == 'value':
                known_values.add(item['name'])
                
        for item in enum['values']:
            if item['type'] == 'comment':
                output.append(f"    {item['text']}")
            elif item['type'] == 'value':
                val_name = item['name']
                val_expr = item['value']
                annotation = item['annotation']
                comment = item['comment']
                
                # Rewrite expression to qualify and cast terms
                rewritten_val = rewrite_expression(val_expr, name, known_values, underlying_type)
                
                # If the expression is negative and the underlying type is unsigned, cast it
                if rewritten_val.strip().startswith('-') and underlying_type.startswith('u'):
                    rewritten_val = f"({underlying_type})({rewritten_val})"
                
                # Build deprecation attribute if needed
                dep_attr = ""
                if annotation:
                    msg_match = re.search(r'"([^"]*)"', annotation)
                    if msg_match:
                        msg = msg_match.group(1)
                        dep_attr = f" [[deprecated(\"{msg}\")]]"
                    else:
                        dep_attr = f" [[deprecated(\"{annotation}\")]]"
                        
                if dep_attr:
                    line_str = f"    {val_name}{dep_attr} = {rewritten_val},"
                else:
                    line_str = f"    {val_name} = {rewritten_val},"
                
                if comment:
                    line_str += f" // {comment}"
                    
                output.append(line_str)
                
        output.append("};")
        
        # Generate bitwise operators if it's a flags enum
        if is_flags:
            output.append("")
            output.append(f"// Bitwise operators for {name}")
            output.append(f"inline constexpr {name} operator|({name} lhs, {name} rhs) {{")
            output.append(f"    return static_cast<{name}>(static_cast<{underlying_type}>(lhs) | static_cast<{underlying_type}>(rhs));")
            output.append("}")
            output.append(f"inline constexpr {name} operator&({name} lhs, {name} rhs) {{")
            output.append(f"    return static_cast<{name}>(static_cast<{underlying_type}>(lhs) & static_cast<{underlying_type}>(rhs));")
            output.append("}")
            output.append(f"inline constexpr {name} operator^({name} lhs, {name} rhs) {{")
            output.append(f"    return static_cast<{name}>(static_cast<{underlying_type}>(lhs) ^ static_cast<{underlying_type}>(rhs));")
            output.append("}")
            output.append(f"inline constexpr {name} operator~({name} arg) {{")
            output.append(f"    return static_cast<{name}>(~static_cast<{underlying_type}>(arg));")
            output.append("}")
            output.append(f"inline {name}& operator|=({name}& lhs, {name} rhs) {{")
            output.append("    lhs = lhs | rhs;")
            output.append("    return lhs;")
            output.append("}")
            output.append(f"inline {name}& operator&=({name}& lhs, {name} rhs) {{")
            output.append("    lhs = lhs & rhs;")
            output.append("    return lhs;")
            output.append("}")
            output.append(f"inline {name}& operator^=({name}& lhs, {name} rhs) {{")
            output.append("    lhs = lhs ^ rhs;")
            output.append("    return lhs;")
            output.append("}")
            
        if i < len(enums) - 1 or classes:
            output.append("")
            
    # Output classes
    for i, cls in enumerate(classes):
        name = cls['name']
        is_removed = cls['is_removed']
        
        dep_attr = " [[deprecated(\"Removed in SteamKit\")]]" if is_removed else ""
        output.append(f"struct{dep_attr} {name} {{")
        
        # Output MsgType static constexpr
        if cls['msg_type_enum'] and cls['msg_type_val']:
            enum_type = cls['msg_type_enum']
            enum_val = cls['msg_type_val']
            output.append(f"    static constexpr {enum_type} MsgType = {enum_type}::{enum_val};")
            if cls['fields']:
                output.append("")
                
        for field in cls['fields']:
            if field['type'] == 'comment':
                output.append(f"    {field['text']}")
            elif field['type'] == 'const':
                val_str = f" = {field['value']}" if field['value'] is not None else ""
                line_str = f"    static constexpr {field['cpp_type']} {field['name']}{val_str};"
                if field['comment']:
                    line_str += f" // {field['comment']}"
                output.append(line_str)
            elif field['type'] == 'field':
                val_str = f" = {field['value']}" if field['value'] is not None else ""
                line_str = f"    {field['cpp_type']} {field['name']}{val_str};"
                if field['comment']:
                    line_str += f" // {field['comment']}"
                output.append(line_str)
                
        output.append("};")
        if i < len(classes) - 1:
            output.append("")
            
    output.append("")
    output.append(f"}} // namespace {namespace_name}")
    output.append("")
    
    return "\n".join(output)

def main():
    parser = argparse.ArgumentParser(description="Parse SteamKit .steamd files and produce C++ enum and class headers.")
    parser.add_argument("-i", "--input", default="SteamKit/Resources/SteamLanguage", help="Directory containing .steamd files")
    parser.add_argument("-o", "--output", default="SteamKit/Resources/SteamLanguage/cpp", help="Output directory for C++ header files")
    parser.add_argument("-n", "--namespace", default="SteamKit", help="C++ namespace to wrap enums and classes in")
    
    args = parser.parse_args()
    
    if not os.path.exists(args.input):
        print(f"Error: Input directory '{args.input}' does not exist.", file=sys.stderr)
        sys.exit(1)
        
    os.makedirs(args.output, exist_ok=True)
    
    # Process all .steamd files in the input directory
    files_processed = 0
    for file_name in sorted(os.listdir(args.input)):
        if file_name.endswith('.steamd'):
            file_path = os.path.join(args.input, file_name)
            imports, enums, classes = parse_steamd_file(file_path)
            
            # Generate a header if the file defines enums or classes
            if enums or classes:
                cpp_content = generate_cpp_header(imports, enums, classes, args.namespace)
                base_name = os.path.splitext(file_name)[0]
                output_file = os.path.join(args.output, f"{base_name}.h")
                
                with open(output_file, 'w', encoding='utf-8') as f:
                    f.write(cpp_content)
                print(f"Generated {output_file} ({len(enums)} enums, {len(classes)} classes)")
                files_processed += 1
                
    print(f"Done. Processed {files_processed} files with enums/classes.")

if __name__ == '__main__':
    main()
