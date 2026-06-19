import os

TARGET_EXTENSIONS = {'.cpp', '.h', '.hpp', '.inl'}
TARGET_DIRECTORIES = ['application', 'engine']

def convert_to_utf8_nombom(file_path):
    try:
        with open(file_path, 'rb') as f:
            content = f.read()
        
        # BOM（0xEF, 0xBB, 0xBF）が存在する場合は除去、なければそのままデコード
        if content.startswith(b'\xef\xbb\xbf'):
            text = content[3:].decode('utf-8', errors='replace')
        else:
            # Shift-JIS等で保存されていた場合も、一度デコードしてUTF-8（BOMなし）へ変換対象とする
            try:
                text = content.decode('utf-8')
            except UnicodeDecodeError:
                try:
                    text = content.decode('cp932', errors='replace')
                except Exception:
                    print(f"[ERROR] スキップ（解析不可）: {file_path}")
                    return False

        # UTF-8（シグネチャなし）で書き出し
        with open(file_path, 'w', encoding='utf-8', newline='') as f:
            f.write(text)
        
        print(f"[CONVERTED] {file_path}")
        return True

    except Exception as e:
        print(f"[FAILED] エラー発生 {file_path}: {e}")
        return False

def main():
    print("文字コードを UTF-8 (シグネチャなし) に変換中...")
    converted_count = 0
    
    for target_dir in TARGET_DIRECTORIES:
        if not os.path.exists(target_dir):
            continue
            
        for root, _, files in os.walk(target_dir):
            for file in files:
                ext = os.path.splitext(file)[1].lower()
                if ext in TARGET_EXTENSIONS:
                    file_path = os.path.join(root, file)
                    if convert_to_utf8_nombom(file_path):
                        converted_count += 1

    print(f"\n完了: {converted_count} 個のファイルを変換しました。")

if __name__ == "__main__":
    main()