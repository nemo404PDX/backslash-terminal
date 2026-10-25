"""Reject ancestor FrameworkElements used as Content data: creates a XAML parent cycle."""
from pathlib import Path
import re
import xml.etree.ElementTree as ET
import sys
root=Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).resolve().parents[2]
x='{http://schemas.microsoft.com/winfx/2006/xaml}'
count=0
for path in (root/'src/cascadia/TerminalSettingsEditor').glob('*.xaml'):
 tree=ET.parse(path)
 def visit(node, ancestors):
  global count
  names=ancestors|({node.attrib[x+'Name']} if x+'Name' in node.attrib else set())
  content=node.attrib.get('Content','')
  match=re.search(r'ElementName\s*=\s*([A-Za-z_][A-Za-z_0-9]*)',content)
  assert not(match and match.group(1) in names), f'{path.name}: ancestor {match.group(1)} assigned as Content'
  count+=1
  for child in node: visit(child,names)
 visit(tree.getroot(),set())
page=ET.parse(root/'src/cascadia/TerminalSettingsEditor/Appearances.xaml')
template=next(e for e in page.iter() if e.attrib.get(x+'Key')=='BezelEdgeControlsTemplate')
assert template.attrib[x+'DataType']=='local:AppearanceViewModel'
control=next(e for e in page.iter() if e.attrib.get('ContentTemplate')=='{StaticResource BezelEdgeControlsTemplate}')
assert control.attrib['Content']=='{x:Bind Appearance, Mode=OneWay}'
print(f'PASS: {count} XAML elements; no ancestor Content; edge template binds non-UI AppearanceViewModel')
