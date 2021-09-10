import nanobench = nanobench%lib{nanobench}
import doctest = doctest%lib{doctest}
import rapidyaml = rapidyaml%lib{rapidyaml}

lib{pulmotor} : src/pulmotor/cxx{stream archive util yaml_util} src/pulmotor/hxx{*} $doctest $rapidyaml
{
	cxx.export.poptions += "-I$src_root/src"
}

./: lib{pulmotor} tests/
