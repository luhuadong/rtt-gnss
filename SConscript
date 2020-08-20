from building import *
Import('rtconfig')

src   = []
cwd   = GetCurrentDir()

# add gps src files.
if GetDepend('PKG_USING_GPS'):
    src += Glob('src/gps.c')
    src += Glob('src/sensor_unknown_gps.c')

if GetDepend('PKG_USING_GPS_SAMPLE'):
    src += Glob('examples/gps_sample.c')
    src += Glob('examples/sensor_gps_sample.c')

# add gps include path.
path  = [cwd + '/inc']

# add src and include to group.
group = DefineGroup('gps', src, depend = ['PKG_USING_GPS'], CPPPATH = path)

Return('group')
