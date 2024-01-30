# vim: set ts=2 sw=2 tw=99 noet ft=python:
import subprocess, sys  

argv = sys.argv[3:]
cwd = sys.argv[1]
print('CMD: ' + ' '.join(argv))
print('CWD: ' + cwd)
print('OUT: ' + sys.argv[2])
open(sys.argv[2], 'a').close()

args = { 'args': ' '.join(argv),
          'stdout': subprocess.PIPE,
          'stderr': subprocess.PIPE,
          'shell':  True,
          'cwd': cwd }
p = subprocess.Popen(**args)
stdout, stderr = p.communicate()
print(stdout.decode("utf-8"))
if p.returncode != 0:
  print(stderr.decode("utf-8"))
sys.exit(p.returncode)