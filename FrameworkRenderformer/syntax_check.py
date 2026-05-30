#!/usr/bin/env python3
"""Syntax check all modified files."""
import py_compile
import sys

files = [
    'local_renderformer/models/renderformer.py',
    'local_renderformer/models/view_transformer.py',
    'local_renderformer/layers/attention.py',
]

ok = True
for f in files:
    try:
        py_compile.compile(f, doraise=True)
        print(f"OK: {f}")
    except py_compile.PyCompileError as e:
        print(f"FAIL: {f} -> {e}")
        ok = False

if ok:
    print("\nAll files have valid syntax!")
else:
    print("\nSome files have syntax errors!")
    sys.exit(1)
