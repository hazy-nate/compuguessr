
import subprocess

def get_definitions(header_path: str):
    """Uses GCC to isolate user macros via diffing, then natively resolves them."""

    cmd_base = ['gcc', '-dM', '-E', '-']
    try:
        res_base = subprocess.run(cmd_base, input='', capture_output=True, text=True, check=True)
    except subprocess.CalledProcessError as e:
        print(f"[-] GCC Baseline failed: {e.stderr}")
        exit(1)

    builtins = set()
    for line in res_base.stdout.splitlines():
        if line.startswith('#define '):
            parts = line.split()
            if len(parts) >= 2:
                builtins.add(parts[1].split('(')[0])

    cmd_target = ['gcc', '-dM', '-E', header_path]
    try:
        res_target = subprocess.run(cmd_target, capture_output=True, text=True, check=True)
    except subprocess.CalledProcessError as e:
        print(f"[-] GCC Target parsing failed: {e.stderr}")
        exit(1)

    user_macro_names = []
    for line in res_target.stdout.splitlines():
        if line.startswith('#define '):
            parts = line.split()
            if len(parts) >= 2:
                name = parts[1]
                # Keep it ONLY if it's not a built-in, and not a function-macro like BS_WORD(x)
                if name.split('(')[0] not in builtins and '(' not in name:
                    user_macro_names.append(name)

    dummy_c_lines = [f'#include "{header_path}"']
    for name in user_macro_names:
        dummy_c_lines.append(f'RESOLVED_{name} = {name}')

    dummy_c_code = '\n'.join(dummy_c_lines)

    cmd_resolve = ['gcc', '-E', '-P', '-']
    try:
        res_resolve = subprocess.run(cmd_resolve, input=dummy_c_code, capture_output=True, text=True, check=True)
    except subprocess.CalledProcessError as e:
        print(f"[-] GCC Resolution pass failed: {e.stderr}")
        exit(1)

    final_macros = {}
    for line in res_resolve.stdout.splitlines():
        if line.startswith('RESOLVED_'):
            # Split "RESOLVED_FCGI_MAXTYPE = 11"
            parts = line.split('=', 1)
            if len(parts) == 2:
                name = parts[0].replace('RESOLVED_', '').strip()
                val = parts[1].strip()

                try:
                    final_macros[name] = int(val, 0)
                except ValueError:
                    final_macros[name] = val

    return final_macros
