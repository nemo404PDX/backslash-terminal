#!/usr/bin/env python3
# Copyright (c) Backslash Terminal contributors.
# Licensed under the MIT license.
"""Reproduce this cleanup against published Git bytes, without shell decoding.

This is a pinned before/after audit, not a rule that future translations may never
be added. Test-ResourceFallbacks.py is the ongoing consistency check.
"""
from pathlib import Path
from collections import Counter
import argparse
import os
import subprocess
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[3]
COMPONENTS = {'TerminalApp': 'TerminalApp', 'Microsoft.Terminal.Control': 'TerminalControl',
              'Microsoft.Terminal.Settings.Editor': 'TerminalSettingsEditor'}

def parse(raw):
    # Parse original UTF-8 bytes, preserving XML comments. XML itself normalizes
    # line endings, independent of PowerShell or the console's active code page.
    return ET.fromstring(raw, parser=ET.XMLParser(target=ET.TreeBuilder(insert_comments=True)))

def semantic(node, preserve=False):
    comment = node.tag is ET.Comment
    preserve = preserve or comment or node.tag in ('value', 'comment') or node.get('{http://www.w3.org/XML/1998/namespace}space') == 'preserve'
    def text(value):
        return value if preserve or (value and value.strip()) else None
    return ('#comment' if comment else node.tag, tuple(sorted(node.attrib.items())), text(node.text),
            tuple((semantic(child, preserve), text(child.tail)) for child in node))

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline', default='v0.0.1-preview.2')
    args = parser.parse_args()
    native = Path(os.environ.get('ProgramFiles', r'C:\Program Files')) / 'Git/cmd/git.exe'
    git = str(native) if native.is_file() else 'git'
    def read(*arguments):
        return subprocess.run([git, '--no-optional-locks', '-C', str(ROOT), *arguments], check=True, capture_output=True).stdout
    revision = read('rev-parse', '--verify', '--end-of-options', args.baseline + '^{commit}').decode('ascii').strip()
    expected = Counter()
    for line in (ROOT / 'docs/backslash/RESOURCE-KEY-AUDIT.md').read_text(encoding='utf-8-sig').splitlines():
        fields = [part.strip() for part in line.split('|')]
        if len(fields) == 5 and fields[1] in COMPONENTS:
            expected[(COMPONENTS[fields[1]], fields[2].strip('`'))] = int(fields[3])
    assert len(expected) == 60 and sum(expected.values()) == 93, 'audit ledger must enumerate all 60 identities / 93 entries'
    removed = Counter()
    files = changed = 0
    for component in COMPONENTS.values():
        resources = ROOT / 'backslash/src/cascadia' / component / 'Resources'
        for path in sorted(resources.glob('*/Resources.resw')):
            relative = path.relative_to(ROOT).as_posix()
            before = parse(read('show', revision + ':' + relative))
            after = parse(path.read_bytes())
            old = {item.get('name'): item for item in before.findall('data')}
            new = {item.get('name'): item for item in after.findall('data')}
            assert not new.keys() - old.keys(), f'{relative}: unexpected added data'
            missing = old.keys() - new.keys()
            for name in missing:
                assert (component, name) in expected, f'{relative}: unexpected removal {name}'
                removed[(component, name)] += 1
                before.remove(old[name])
            assert all(semantic(old[name]) == semantic(item) for name, item in new.items()), f'{relative}: surviving translation/comment changed'
            assert semantic(before) == semantic(after), f'{relative}: non-data XML, comments, order or headers changed'
            files += 1
            changed += bool(missing)
    assert removed == expected, f'removal identities/counts differ: missing={expected-removed}, extra={removed-expected}'
    print(f'PASS: {revision}; {files} XML files compared; exactly 93 removed entries in {changed} files / 60 identities; all surviving translations/comments and non-data XML unchanged')

if __name__ == '__main__':
    main()
