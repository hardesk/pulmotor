import nanobench = nanobench%lib{nanobench}
import doctest = doctest%lib{doctest}

lib{pulmotor} : src/pulmotor/cxx{stream archive util} src/pulmotor/hxx{*} $doctest
{
	cxx.export.poptions += "-I$src_root/src"
}

./: lib{pulmotor} tests/
