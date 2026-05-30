#!/usr/bin/env python3
"""Check torch availability."""
try:
    import torch
    print(f"torch {torch.__version__} at {torch.__file__}")
except ImportError:
    print("no-torch-in-default-python")
