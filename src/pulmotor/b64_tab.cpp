#include <cstdio>
#include <cstring>

char const* enc =
"ABCDEFGHIJKLMNOP"
"QRSTUVWXYZabcdef"
"ghijklmnopqrstuv"
"wxyz0123456789+/";

int main(int narg, char** parg) {
	using namespace std;

	bool dbg = false;

	if (narg > 1)
		if (strcmp(parg[1], "-d") == 0) dbg = true;

	for (int i=0; i<256; ++i) {
		char const* p = strchr(enc, i);
		size_t v = 0;

		if(p==enc+strlen(enc)) v = 0;
		else if (p) v = p - enc;
		else if (i=='-') v = 62; // '-' is same as '+'
		else if (i=='_') v = 63; // '_' is same as '/'
		else if (i=='=') v = 0;

		printf("%2d", (int)v);
		if (dbg)
			printf(" /* %c*/", (i > 32 && i < 127) || i >= 160 ? i : '?');

		if (i != 255) printf(",");
		printf((i+1) % 16 == 0 && i != 0 ? "\n" : " ");
	}

	return 0;
}
