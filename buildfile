import nanobench = nanobench%lib{nanobench}
import doctest = doctest%lib{doctest}
import rapidyaml = rapidyaml%lib{rapidyaml}

info "PULMOTOR buildfile: src_root: $src_root, src_base: $src_base, out_base: $out_base, out_root: $out_root, import.target: $import.target, copts:" $(config.cxx.coptions)


lib{pulmotor} : src/pulmotor/cxx{stream archive util yaml_util} src/pulmotor/hxx{*} $doctest $rapidyaml
{
	cxx.export.poptions += "-I$src_root/src"
}

./: lib{pulmotor} tests/
