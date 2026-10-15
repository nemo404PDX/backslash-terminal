# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
"""Check startup-picker source constraints. This is not native XAML layout/UI testing."""
from pathlib import Path
import argparse
import json
import xml.etree.ElementTree as ET


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--evidence-directory", type=Path)
    args = parser.parse_args()
    editor = args.repo / "src/cascadia/TerminalSettingsEditor"
    root = ET.parse(editor / "Launch.xaml").getroot()
    ui = "{http://schemas.microsoft.com/winfx/2006/xaml/presentation}"
    x = "{http://schemas.microsoft.com/winfx/2006/xaml}"
    local = "{using:Microsoft.Terminal.Settings.Editor}"
    checks = []

    def require(condition, label):
        if not condition:
            raise AssertionError(label)
        checks.append(label)

    def named(scope, name):
        matches = [n for n in scope.iter() if n.get(x + "Name") == name]
        require(len(matches) == 1, "unique named element: " + name)
        return matches[0]

    card = named(root, "StartupGroup")
    template = card.find(local + "SettingContainer.Template/" + ui + "ControlTemplate")
    require(template is not None, "startup card overrides the horizontal default template")
    grid = template.find(ui + "Grid")
    columns = grid.find(ui + "Grid.ColumnDefinitions")
    require(len(columns) == 1 and columns[0].get("Width") == "*", "card has one available-width column")
    rows = grid.find(ui + "Grid.RowDefinitions")
    require(len(rows) == 3 and all(n.get("Height") == "Auto" for n in rows), "heading, description and content have separate automatic-height rows")
    require(grid.get("MinWidth") == "0" and grid.get("Padding") == "16,12,16,12", "card can shrink with available width, retaining symmetric padding")
    require(not any(k in grid.attrib for k in ("Width", "Height", "MaxHeight")), "card does not force fixed dimensions or height")
    help_text = named(template, "HelpTextBlock")
    require(help_text.get("Grid.Row") == "1" and help_text.get("TextWrapping") == "Wrap", "description wraps independently above the field")
    presenter = template.find(ui + "Grid/" + ui + "ContentPresenter")
    require(presenter.get("Grid.Row") == "2" and presenter.get("HorizontalAlignment") == "Stretch", "content receives a dedicated full-width row")
    require(presenter.get("Content") == "{TemplateBinding Content}", "card preserves SettingContainer content semantics")
    named(template, "ResetButton")
    picker = named(card, "StartupGroupPicker")
    require(picker.get("HorizontalAlignment") == "Stretch" and picker.get("MinWidth") == "0", "picker stretches without a competing minimum")
    require(not any(k in picker.attrib for k in ("Width", "MaxWidth", "Height")), "picker width/height follows available card space")
    require(picker.get("UpdateTextOnSelect") == "False" and "TextMemberPath" not in picker.attrib, "explicit safe selection presentation retained")
    require(picker.get("TextChanged") == "StartupGroup_TextChanged" and picker.get("QuerySubmitted") == "StartupGroup_QuerySubmitted", "search and stable-ID submission handlers retained")
    require(picker.get("ToolTipService.ToolTip") == "{Binding Text, RelativeSource={RelativeSource Self}}", "selected full text has a native self-bound tooltip")
    text_style = next(n for n in root.iter(ui + "Style") if n.get(x + "Key") == "StartupGroupSearchTextBoxStyle")
    setters = {n.get("Property"): n.get("Value") for n in text_style.findall(ui + "Setter")}
    require(setters.get("HorizontalAlignment") == "Stretch" and setters.get("MinWidth") == "0", "inner textbox also fills the row")
    require(not set(setters).intersection(("Width", "MaxWidth", "Height", "MaxHeight")), "inner textbox has no fixed width/height or scaling cap")
    item_style = picker.find(ui + "AutoSuggestBox.ItemContainerStyle/" + ui + "Style")
    require(any(n.get("Property") == "HorizontalContentAlignment" and n.get("Value") == "Stretch" for n in item_style.findall(ui + "Setter")), "suggestion container constrains content to popup width")
    item = picker.find(ui + "AutoSuggestBox.ItemTemplate/" + ui + "DataTemplate")
    require(item.get(x + "DataType") == "local:EnumEntry", "existing typed/stringable suggestion type retained")
    label = item.find(ui + "TextBlock")
    require(label.get("TextWrapping") == "Wrap" and label.get("HorizontalAlignment") == "Stretch", "long and unbroken suggestion names wrap")
    require("MaxLines" not in label.attrib and "TextTrimming" not in label.attrib, "suggestion names have no stored/display ellipsis limit")
    require(label.get("ToolTipService.ToolTip") == "{x:Bind EnumName, Mode=OneWay}" and label.get("AutomationProperties.Name") == "{x:Bind EnumName, Mode=OneWay}", "suggestions retain complete tooltip and accessibility names")
    status = named(card, "StartupGroupStatus")
    require(status.get("TextWrapping") == "Wrap" and status.get("AutomationProperties.LiveSetting") == "Polite", "status wraps inside the card with polite announcements")
    resources = {n.get("name"): n.findtext("value") for n in ET.parse(editor / "Resources/en-US/Resources.resw").getroot().findall("data")}
    require(resources["StartupGroups_Picker.AutomationProperties.Name"] == "Startup group", "field accessibility label preserved")
    require(len(resources["StartupGroups_Group.HelpText"]) <= 100 and len(resources["StartupGroups_Mode.HelpText"]) <= 130, "descriptions remain concise")
    matrix = []
    # Algebra from the inspected single-column/padding constraints only.
    # Font metrics, themes, popup placement and actual Measure/Arrange are not run.
    for width in (240, 320, 640, 1000):
        for dpi in (96, 144, 192):
            for text_scale in (1.0, 1.5, 2.0):
                matrix.append({"availableCardDips": width, "dpi": dpi, "textScale": text_scale,
                               "expectedFieldDips": width - 32,
                               "expectedFieldPixels": (width - 32) * dpi / 96,
                               "descriptionCanReduceFieldWidth": False,
                               "kind": "source constraint allocation; not native measurement"})
    evidence = {"checks": checks, "sourceConstraintMatrix": matrix,
                "nativeVisualLayout": "UNRUN: no native XAML measurement, font/text-scale/DPI change, screenshot or picker interaction performed.",
                "limitations": "Checks catch source regressions in row separation, stretch, dimensions and full-name access. They do not prove native popup placement, theme/font layout or crash-fix acceptance."}
    if args.evidence_directory:
        args.evidence_directory.mkdir(parents=True, exist_ok=True)
        (args.evidence_directory / "startup-picker-layout-source-checks.json").write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(f"PASS: {len(checks)} source constraints; {len(matrix)} hypothetical width/DPI/text-scale allocations. Native layout/interaction UNRUN.")


if __name__ == "__main__":
    main()
