import re, os, sys
import subprocess

import zipfile
import base64
try:
	import urllib.request as urllib
except ImportError:
	import urllib2 as urllib

project = 'accelerator'

platform = 'unknown'
if sys.platform.startswith('linux'):
	platform = 'linux'
elif sys.platform.startswith('win32'):
	platform = 'windows'
elif sys.platform.startswith('darwin'):
	platform = 'mac'

def GITHash():
	p = subprocess.Popen(['git', 'rev-parse', '--short', 'HEAD'], stdout = subprocess.PIPE, stderr = subprocess.PIPE)
	(stdout, stderr) = p.communicate()
	stdout = stdout.decode('UTF-8')
	return stdout.rstrip('\r\n')

def GITBranch():
	travis_branch = os.environ.get('TRAVIS_BRANCH', False)
	if travis_branch:
		return travis_branch

	appveyor_branch = os.environ.get('APPVEYOR_REPO_BRANCH', False)
	if appveyor_branch:
		return appveyor_branch

	p = subprocess.Popen(['git', 'rev-parse', '--abbrev-ref', 'HEAD'], stdout = subprocess.PIPE, stderr = subprocess.PIPE)
	(stdout, stderr) = p.communicate()
	stdout = stdout.decode('UTF-8')
	return stdout.rstrip('\r\n')

def GITVersion():
	p = subprocess.Popen(['git', 'rev-list', '--count', '--first-parent', 'HEAD'], stdout = subprocess.PIPE, stderr = subprocess.PIPE)
	(stdout, stderr) = p.communicate()
	stdout = stdout.decode('UTF-8')
	return stdout.rstrip('\r\n')

def ReleaseVersion():
	productFile = open('../product.version', 'r')
	productContents = productFile.read()
	productFile.close()

	m = re.match('(\d+)\.(\d+)\.(\d+)(.*)', productContents)
	if m == None:
		raise Exception('Could not detremine product version')

	major, minor, release, tag = m.groups()
	return '.'.join([major, minor, release])

filename = '-'.join([project, ReleaseVersion(), 'git' + GITVersion(), GITHash(), platform])

debug_build = os.environ.get('is_debug_build', False) == "1"

if debug_build:
	filename += '-debug'

filename += '.zip'

zip = zipfile.ZipFile(filename, 'w', zipfile.ZIP_DEFLATED)

for base, dirs, files in os.walk('package'):
	for file in files:
		fn = os.path.join(base, file)
		fns = fn[(len('package') + 1):]

		zip.write(fn, fns)

print("%-33s %-10s %21s %12s" % ("File Name", "CRC32", "Modified      ", "Size"))
for zinfo in zip.infolist():
	date = "%d-%02d-%02d %02d:%02d:%02d" % zinfo.date_time[:6]
	print("%-33s %-10d %21s %12d" % (zinfo.filename, zinfo.CRC, date, zinfo.file_size))

zip.close()

if 'ftp_username' in os.environ:
	print('')

	request = urllib.Request("https://builds.limetech.io/upload.php?project=%s&branch=%s&filename=%s" % (project, GITBranch(), filename), open(filename, 'rb'), headers={
		'Authorization': "Basic %s" % (base64.encodestring(("%s:%s" % (os.environ['ftp_username'], os.environ['ftp_password'])).encode()).decode()[:-1]),
		'Content-Type': 'application/zip',
    	'Content-Length': os.stat(filename).st_size,
	})
	print(urllib.urlopen(request).read().decode('utf-8'))
	
	print('Uploaded as \'' + filename + '\'')
