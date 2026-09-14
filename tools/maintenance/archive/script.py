import json
with open('apps_code.txt', 'r', encoding='utf-8') as f:
    for line in f:
        calls = json.loads(line)
        for call in calls:
            if call['name'] in ('replace_file_content', 'multi_replace_file_content'):
                for i, c in enumerate(call['args']['ReplacementChunks']):
                    print(f"CHUNK {i}:")
                    print("TARGET:")
                    print(c["TargetContent"])
                    print("REPLACE:")
                    print(c["ReplacementContent"])
                    print("---")
