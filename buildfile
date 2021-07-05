
#lib{pulmotor} : src/pulmotor/cxx{*} src/pulmotor/hxx{*}
lib{pulmotor} : src/pulmotor/cxx{stream archive util} src/pulmotor/hxx{*}
{
	cxx.export.poptions += "-I$src_root/src"
}
#doctest%lib{doctest}

./: lib{pulmotor} tests/

