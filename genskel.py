#!/usr/bin/env python

import os


def gen(file):
    """
    Generate skeleton for a single file
    :param file: file path
    """
    with open(file, 'r', encoding='utf-8') as f:
        copying = True
        lines = []
        for line in f.readlines():
            if 'TODO:' in line:
                # Stop copying code in the quiz part
                copying = False
                lines.append(line)
            elif 'End for ' in line:
                copying = True
            elif copying:
                lines.append(line)
    with open(file, 'w', encoding='utf-8') as f:
        f.writelines(lines)


def main():
    workdir = os.path.dirname(os.path.abspath(__file__))
    print(f"Start making skeleton in {os.path.join(workdir, 'src')}...")
    for subdir, dirs, files in os.walk(os.path.join(workdir, 'src')):
        for file in files:
            gen(os.path.join(subdir, file))


if __name__ == '__main__':
    main()
