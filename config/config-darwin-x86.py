CXX			   = 'g++'
CXXFLAGS       = ['-arch', 'i386', '-mmacosx-version-min=10.5', '-march=nocona', '-msse2', '-mfpmath=sse', '-ftree-vectorize', '-funsafe-math-optimizations', '-fno-rounding-math', '-fno-signaling-nans', '-fomit-frame-pointer', '-isysroot', '/Developer/SDKs/MacOSX10.5.sdk', '-O3', '-Wall', '-g', '-pipe', '-DMTS_DEBUG', '-DSINGLE_PRECISION', '-DMTS_SSE', '-DMTS_HAS_COHERENT_RT', '-fopenmp', '-fstrict-aliasing', '-fsched-interblock', '-freorder-blocks']
LINKFLAGS      = ['-framework', 'OpenGL', '-framework', 'Cocoa', '-arch', 'i386', '-mmacosx-version-min=10.5', '-Wl,-syslibroot,/Developer/SDKs/MacOSX10.5.sdk']
BASEINCLUDE    = ['#include']
BASELIB        = ['m', 'pthread', 'gomp']
OEXRINCLUDE    = ['#dependencies/darwin/OpenEXR.framework/Headers/OpenEXR']
OEXRLIBDIR     = ['#dependencies/darwin/OpenEXR.framework/Resources/lib']
OEXRLIB        = ['Half', 'IlmImf', 'Iex', 'Imath', 'z']
PNGINCLUDE     = ['#dependencies/darwin/libpng.framework/Headers']
PNGLIBDIR      = ['#dependencies/darwin/libpng.framework/Resources/lib']
PNGLIB         = ['png']
JPEGINCLUDE    = ['#dependencies/darwin/libjpeg.framework/Headers']
JPEGLIBDIR     = ['#dependencies/darwin/libjpeg.framework/Resources/lib']
JPEGLIB        = ['jpeg']
XERCESINCLUDE  = ['#dependencies/darwin/Xerces-C.framework/Headers']
XERCESLIBDIR   = ['#dependencies/darwin/Xerces-C.framework/Resources/lib']
XERCESLIB      = ['xerces-c']
GLINCLUDE      = ['#dependencies/darwin/GLEW.framework/Headers']
GLLIBDIR       = ['#dependencies/darwin/GLEW.framework/Resources/libs']
GLLIB          = ['GLEW', 'objc']
GLFLAGS        = ['-DGLEW_MX']
BOOSTINCLUDE   = ['#dependencies']
BOOSTLIB       = ['boost_filesystem', 'boost_system']
BOOSTLIBDIR    = ['#dependencies/darwin/libboost.framework/Resources/lib']
COLLADAINCLUDE = ['#dependencies/windows/include/colladadom', '#dependencies/windows/include/colladadom/1.4']
COLLADALIB     = ['libCollada14Dom']
COLLADALIBDIR  = ['#dependencies/darwin/Collada14Dom.framework/Resources/lib']
