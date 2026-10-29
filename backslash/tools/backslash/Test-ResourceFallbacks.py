#!/usr/bin/env python3
# Copyright (c) Backslash Terminal contributors.
# Licensed under the MIT license.
"""Validate neutral fallback coverage; never remove or synthesize translations."""
from pathlib import Path
import sys
import xml.etree.ElementTree as ET
import tempfile

COMPONENTS = ('TerminalApp', 'TerminalControl', 'TerminalSettingsEditor')

def check(root):
    errors, files, entries = [], 0, 0
    for component in COMPONENTS:
        resources = root / 'src/cascadia' / component / 'Resources'
        neutral = resources / 'en-US/Resources.resw'
        if not neutral.is_file():
            errors.append(f'{neutral}: missing en-US fallback file')
            continue
        names = {d.get('name') for d in ET.parse(neutral).getroot().findall('data')}
        for path in sorted(resources.glob('*/Resources.resw')):
            seen = set()
            files += 1
            for data in ET.parse(path).getroot().findall('data'):
                name = data.get('name')
                entries += 1
                if name in seen:
                    errors.append(f'{path}: duplicate {name}')
                seen.add(name)
                if name not in names:
                    errors.append(f'{path}: no en-US fallback for {name}')
    return errors, files, entries

def self_test():
    with tempfile.TemporaryDirectory(prefix='backslash-resource-check-') as temporary:
        root = Path(temporary)
        for component in COMPONENTS:
            for locale in ('en-US', 'test-locale'):
                folder = root / 'src/cascadia' / component / 'Resources' / locale
                folder.mkdir(parents=True)
                (folder / 'Resources.resw').write_text('<root><data name="active"><value>text</value></data></root>')
        assert not check(root)[0], 'valid translated key rejected'
        path = root / 'src/cascadia/TerminalApp/Resources/test-locale/Resources.resw'
        path.write_text('<root><data name="obsolete"><value>text</value></data></root>')
        assert any('no en-US fallback for obsolete' in error for error in check(root)[0]), 'missing fallback accepted'
        path.write_text('<root><data name="active"/><data name="active"/></root>')
        assert any('duplicate active' in error for error in check(root)[0]), 'duplicate key accepted'
        path.write_text('<broken')
        try:
            check(root)
        except ET.ParseError:
            pass
        else:
            raise AssertionError('malformed resource accepted')
    print('PASS: synthetic valid translation, missing fallback, duplicate and malformed-resource checks')

if __name__ == '__main__':
    if '--self-test' in sys.argv:
        self_test()
    root = Path(__file__).resolve().parents[2]
    try:
        errors, files, entries = check(root)
    except (ET.ParseError, OSError) as exc:
        sys.exit(f'FAIL: invalid resource input: {exc}')
    for error in errors:
        print(f'FAIL: {error}')
    if errors:
        sys.exit(1)
    print(f'PASS: {files} resource files, {entries} entries; localized names have unique en-US fallbacks')
