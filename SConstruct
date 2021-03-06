import sys
import os
import subprocess
from os.path import join, dirname, abspath
from types import DictType, StringTypes
root_dir = dirname(File('SConstruct').rfile().abspath)
sys.path.append(join(root_dir, 'tools'))
import js2c

if ARGUMENTS.get('debug', 0):
  node_exe ='node_g'
else:
  node_exe ='node'

def shellOut(target):
  return subprocess.Popen(target, stdout=subprocess.PIPE).communicate()[0].strip()

def CheckForNodeJS(context):
  context.Message('Checking for node.js ...')
  result = shellOut(['which', node_exe]) != ''
  context.Result(result)
  return result

using_node_js = (('libxmljs.node' in COMMAND_LINE_TARGETS) or ('test' in COMMAND_LINE_TARGETS))

libs = ['xml2']
libpath = [
  '/opt/local/lib',
  '/usr/local/lib',
  '/usr/lib'
]
cflags = ' '.join([
  '-I/opt/local/include',
  '-I/opt/local/include/libxml2',
  '-I/usr/local/include',
  '-I/usr/local/include/libxml2',
  '-I/usr/include',
  '-I/usr/include/libxml2'
])

if using_node_js:
  cflags += ' ' + shellOut([node_exe, '--cflags'])

testBuilder = Builder(action = 'node spec/tacular.js')

env = Environment(BUILDERS = {'Test' : testBuilder})
env.Append(
  LIBPATH = libpath,
  CCFLAGS = cflags
)

if not env.GetOption('clean'):
  conf = Configure(env, custom_tests = {'CheckForNodeJS' : CheckForNodeJS})
  print conf.CheckForNodeJS()
  if not conf.CheckLib('xml2', header = '#include <libxml/parser.h>', language = 'c++'):
    print 'Did not find libxml2, exiting!'
    Exit(1)
  if not using_node_js and not conf.CheckLib('v8', header = '#include <v8.h>', language = 'c++'):
    print 'Did not find libv8, exiting!'
    Exit(1)
  if using_node_js and not conf.CheckForNodeJS():
    print 'Did not find node.js exiting!'
    Exit(1)
  env = conf.Finish()

# Build native js
js2c.JS2C(Glob('src/*.js'), ['src/natives.h'])

cc_sources = Glob('src/*.cc')

# Build libxmljs binary
libxmljs = env.Program(
  target = 'libxmljs',
  source = cc_sources,
  CCFLAGS = cflags,
  LIBS = libs + ['v8'],
  LIBPATH = libpath
)

# Build libxmljs node plugin
if env['PLATFORM'] == 'darwin':
  env.Append(LDMODULEFLAGS = ['-bundle_loader', shellOut(['which', 'node'])])

node = env.LoadableModule(
  target = 'libxmljs.node',
  source = cc_sources,
  CCFLAGS = cflags,
  LIBS = libs,
  LIBPATH = libpath
)

# Run tests
tests = env.Test('test', 'libxmljs.node')

env.Default(libxmljs)
