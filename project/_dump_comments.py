import sys
with open(sys.argv[1], 'r', encoding='utf-8') as f:
    lines = f.readlines()

start, end = int(sys.argv[2]), int(sys.argv[3])
with open(sys.argv[4], 'w', encoding='utf-8') as out:
    for i in range(start-1, min(end, len(lines))):
        s = lines[i].rstrip('\n')
        if s.strip():
            out.write(f'L{i+1}: {s[:150]}\n')
print(f'Written lines {start}-{end}')
