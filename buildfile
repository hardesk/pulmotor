
#lib{pulmotor} : src/pulmotor/cxx{*} src/pulmotor/hxx{*}
lib{pulmotor} : src/pulmotor/cxx{stream archive} src/pulmotor/hxx{*}
{
	cxx.export.poptions += "-I$out_root/src"
}
#doctest%lib{doctest}

./: lib{pulmotor} tests/

