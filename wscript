
#
# This file is the default set of rules to compile a Pebble project.
#
# Feel free to customize this to your needs.
#

import os.path
from sh import uglifyjs
from sh import jshint
from waflib.Configure import conf

top = '.'
out = 'build'

def concatenate_js(task):
    inputs = (input.abspath() for input in task.inputs)
    uglifyjs(*inputs, o=task.outputs[0].abspath())

def ping(ctx):
    print(' ping! %d' % id(ctx))
    print(' ctx.path.abspath() = %s' % ctx.path.abspath())
    print('paths = ', ctx.path.ant_glob('src/js' + '/**/*.js'))
    print ctx


def options(ctx):
    ctx.load('pebble_sdk')

def configure(ctx):
    ctx.load('pebble_sdk')

def concat(ctx):
    ctx.load('pebble_sdk')
    print "Testing concat JS ..."
    js_target = ctx.concat_javascript(js_path='src/js')    
    print js_target

def build(ctx):
    ctx.load('pebble_sdk')

    # Concat Javascript
    js_target = ctx.concat_javascript(js_path='src/js')

    #print "js_target = ", js_target
    
    #js_target = ctx.path.ant_glob('src/js/pebble-js-app.js')
    

    #print('js_target={}'.format(js_target))

        #ctx.pbl_bundle(binaries=binaries, js=ctx.path.ant_glob('src/js/**/*.js'))


    #ctx(rule='touch ${TGT}', target='foo.txt')
    #ctx(rule='cp ${SRC} ${TGT}', source='foo.txt', target='bar.txt')

    build_worker = os.path.exists('worker_src')
    binaries = []

    #print(' ctx.env.TARGET_PLATFORMS = %s' % ctx.env.TARGET_PLATFORMS)

    for p in ctx.env.TARGET_PLATFORMS:
        #print p
        #print '=========='
        #print ctx.env
        #print '=========='
        #print ctx.all_envs[p]
        ctx.set_env(ctx.all_envs[p])
        app_elf='{}/pebble-app.elf'.format(ctx.env.BUILD_DIR)
        ctx.pbl_program(source=ctx.path.ant_glob('src/**/*.c'),
        target=app_elf)

        if build_worker:
            worker_elf='{}/pebble-worker.elf'.format(ctx.env.BUILD_DIR)
            binaries.append({'platform': p, 'app_elf': app_elf, 'worker_elf': worker_elf})
            ctx.pbl_worker(source=ctx.path.ant_glob('worker_src/**/*.c'),
            target=worker_elf)
        else:
            binaries.append({'platform': p, 'app_elf': app_elf})

    ctx.pbl_bundle(binaries=binaries, js=js_target)

@conf
def concat_javascript(ctx, js_path=None):

    # Unordered list (TBD)
    # js_nodes = (ctx.path.ant_glob(js_path + '/**/*.js', excl='src/js/pebble-js-app.js') +
    #            ctx.path.ant_glob(js_path + '/**/*.json') +
    #            ctx.path.ant_glob(js_path + '/**/*.coffee'))

    # Ordered (Add any JS files here by hand)
    js_nodes = (ctx.path.ant_glob(js_path + '/constants.js') +
                ctx.path.ant_glob(js_path + '/util.js') +
                ctx.path.ant_glob(js_path + '/messages.js') +
                ctx.path.ant_glob(js_path + '/pebble-cal.js'))

    print 'js_nodes = ', js_nodes

    #all_js = "\n".join([node.read() for node in js_nodes])
    #print "all_js = ", all_js

    #print('JS NODES:{}'.format(js_nodes))

    if not js_nodes:
        return []

    def uglify_javascript_task(task):
        print "Uglify JS Task! :::"
        inputs = (input.abspath() for input in task.inputs)
        uglifyjs(*inputs, o=task.outputs[0].abspath())

    def concat_javascript_task(task):
        print "Concat JS Task! :::"
        all_js = "\n".join([node.read() for node in task.inputs])
        task.outputs[0].write(all_js)

    def js_jshint(task):
        print "JSHint run ..."
        inputs = (input.abspath() for input in task.inputs)
        jshint(*inputs, config="./pebble-jshintrc")

    # Unoptimized js
    js_target_ao = ctx.path.make_node('build/src/js/pebble-js-app-ao.js')

    # Optimized js (through uglify)
    js_target = ctx.path.make_node('build/src/js/pebble-js-app.js')


    # JSHint
    ctx(rule=js_jshint, source=js_nodes)

    # Concat JS in a right order
    ctx(name="concat_uglify",
        rule=concat_javascript_task,
        source=js_nodes,
        target=js_target)

    # Uglify all
    #ctx(name="concat_uglify",
    #    rule=uglify_javascript_task,
    #    source=js_target_ao,
    #    target=js_target)

    # Clean unoptimized version
    #ctx(rule='rm ${SRC}',
    #    source=js_target_ao,
    #    after="concat_uglify")

    return js_target



