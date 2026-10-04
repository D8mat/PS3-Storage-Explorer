from pathlib import Path
import os
import subprocess
import tempfile
from support import ENV, compile_test

root = Path(__file__).resolve().parents[1]
env = ENV.copy()
exe = compile_test('test-multi', ['source/scan.c', 'tests/multi_host.c'])
def run(paths, cancel=False):
    e=env.copy()
    if cancel: e['SCAN_CANCEL']='1'
    lines=subprocess.check_output([str(exe), *[str(p).replace(chr(92), '/') for p in paths]],env=e,text=True).splitlines()
    return list(map(int,lines[0].split())),[int(x.split('\t')[0]) for x in lines[1:]]
with tempfile.TemporaryDirectory() as tmp:
    base=Path(tmp); a=base/'GAMES'; b=base/'GAMES_1'; nested=a/'nested'
    nested.mkdir(parents=True); b.mkdir()
    (a/'same.iso').write_bytes(bytes(100)); (b/'same.iso').write_bytes(bytes(200))
    (nested/'small').write_bytes(bytes(50))
    header,sizes=run([a,b]); assert header==[2,0,0,0] and sizes==[200,100,50]
    for paths in ([a,nested,a,b],[nested,a,b,a],[str(a)+'/',a,b]):
        header,sizes=run(paths); assert header[0]==2 and header[1]==len(paths)-2 and sizes==[200,100,50]
    header,sizes=run([base/'missing',b]); assert header==[2,0,1,0] and sizes==[200]
    header,sizes=run([a,b],True); assert header[3]==1 and header[0]==1
    header,sizes=run([]); assert header==[0,0,0,0] and sizes==[]
print('PASS: merged sort, same names, prefix boundaries, overlap in either order, duplicate/trailing slash, missing path, cancellation, empty selection')
