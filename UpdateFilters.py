import os
import xml.etree.ElementTree as ET
import hashlib
import uuid
import sys

# MSBuild XML namespace
NAMESPACE = "http://schemas.microsoft.com/developer/msbuild/2003"
ET.register_namespace('', NAMESPACE)
NS_PREFIX = f"{{{NAMESPACE}}}"

ITEM_TAGS = [
    "ClCompile",
    "ClInclude",
    "FxCompile",
    "None",
    "ResourceCompile",
    "Image",
    "Text",
    "Natvis"
]

def get_deterministic_guid(text: str) -> str:
    md5 = hashlib.md5(text.encode('utf-8')).digest()
    u = uuid.UUID(bytes=md5)
    return f"{{{str(u).upper()}}}"

def convert_to_filter_name(include_path: str) -> str:
    # スラッシュをバックスラッシュに統一
    normalized = include_path.replace("/", "\\")
    # 先頭の ..\ を取り除く
    while normalized.startswith("..\\"):
        normalized = normalized[3:]
    
    directory = os.path.dirname(normalized)
    if not directory or directory == ".":
        return ""
    return directory

def get_filter_ancestors(filter_path: str) -> list:
    parts = filter_path.split("\\")
    ancestors = []
    for i in range(len(parts)):
        ancestors.append("\\".join(parts[:i+1]))
    return ancestors

def write_filters_file(vcxproj_path: str):
    if not os.path.exists(vcxproj_path):
        print(f"Error: Project file not found: {vcxproj_path}")
        return

    print(f"\033[96mProcessing project: {os.path.abspath(vcxproj_path)}\033[0m")
    
    try:
        tree = ET.parse(vcxproj_path)
        root = tree.getroot()
    except Exception as e:
        print(f"Error parsing xml: {e}")
        return

    # アイテムの収集
    project_items = []
    filter_set = set()

    # MSBuild XMLの名前空間を考慮して要素を探す
    for tag in ITEM_TAGS:
        for item in root.findall(f".//{NS_PREFIX}{tag}"):
            include = item.get("Include")
            if not include:
                continue
            
            normalized_include = include.replace("/", "\\")
            filter_name = convert_to_filter_name(include)
            
            project_items.append({
                "tag": tag,
                "include": normalized_include,
                "filter": filter_name
            })
            
            if filter_name:
                for ancestor in get_filter_ancestors(filter_name):
                    filter_set.add(ancestor)

    print(f"  -> Found {len(project_items)} build items (source/header/etc.)")
    print(f"  -> Generated {len(filter_set)} unique filter folder structures")

    # .filters XML の構築
    filters_root = ET.Element(f"{NS_PREFIX}Project", ToolsVersion="4.0")
    
    # 1. Filter定義の追加
    filter_group = ET.SubElement(filters_root, f"{NS_PREFIX}ItemGroup")
    for filter_path in sorted(list(filter_set)):
        filter_el = ET.SubElement(filter_group, f"{NS_PREFIX}Filter", Include=filter_path)
        unique_id_el = ET.SubElement(filter_el, f"{NS_PREFIX}UniqueIdentifier")
        unique_id_el.text = get_deterministic_guid(filter_path)

    # 2. 各アイテムの追加 (Tagごとにまとめる)
    for tag in ITEM_TAGS:
        tag_items = [item for item in project_items if item["tag"] == tag]
        if not tag_items:
            continue
            
        # ソートして追加順を一意にする
        tag_items.sort(key=lambda x: x["include"])
        
        item_group = ET.SubElement(filters_root, f"{NS_PREFIX}ItemGroup")
        for item in tag_items:
            item_el = ET.SubElement(item_group, f"{NS_PREFIX}{item['tag']}", Include=item["include"])
            if item["filter"]:
                filter_el = ET.SubElement(item_el, f"{NS_PREFIX}Filter")
                filter_el.text = item["filter"]

    # 保存
    filters_path = vcxproj_path + ".filters"
    
    # XMLをきれいに整形して保存するための処理
    try:
        ET.indent(filters_root, space="  ", level=0)
    except AttributeError:
        # Python 3.8 以前などの互換性フォールバック
        pass
    
    # XML宣言付きで書き出し
    filters_tree = ET.ElementTree(filters_root)
    try:
        with open(filters_path, "wb") as f:
            f.write(b'<?xml version="1.0" encoding="utf-8"?>\n')
            filters_tree.write(f, encoding="utf-8", xml_declaration=False)
        print(f"  -> \033[92mSuccessfully saved: {filters_path}\033[0m")
    except Exception as e:
        print(f"Error saving file: {e}")

def main():
    # ANSIカラーをWindowsのコンソールで有効にするための処理
    if sys.platform == "win32":
        os.system("")

    default_projects = [
        "application/GameTemplate.vcxproj",
        "engine/KentoCompoEngine.vcxproj"
    ]
    
    for proj in default_projects:
        write_filters_file(proj)

if __name__ == "__main__":
    main()
