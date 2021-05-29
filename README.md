# pulmotor
Another implementation of serialization library for C++17. Mostly this library is an excersize in C++ coding to learn C++17 approaches to type magic.

It actually started as an exersize to make a "blit" serialization where objects are written out in such a way that the resulting block can be loaded whole and (after applying relocations) the memory can be used straight as a runtime data structure. That did work for simple cases, but for more complex structures it seemed to get out of hand pretty quickly and I didn't have the determination to figure out the approaches there. So in order to have something usable in addition to that I also wrote a serializer/deserializer. The latter needs to interpret stream when loading but is way more portable and well, usable. I may come back to `blit` approach later, but only if I find a strong need for it.

The serialization API is similar to boost serialization. Instead though, the library hijacks `|` operator. It has some features which are the main drive for this library:

- you can pass a context to children being serialized (not in current version, with alloc|construct and place modifiers not sure it is needed).
- supports placement new and std::allocator memory allocations for objects when serializing through pointer (again not sure how useful that'll prove to be actually).
- `emplace` support when de-serializing objects.

An example follows:
```
#include <pulmotor/pulmotor.hpp>

struct X
{
	int x;
	template<class Ar> void serialize(Ar& ar) { ar | x; }
};

struct Y
{
	X* px {nullptr};
	~Y() { delete px; }

	void init() { pa = new A(); }

	template<class Ar> void serialize(Ar& ar) { ar | px; }
};


int main()
{
	pulmotor::archive_vector_out ar;

	Y y;
	y.init(100);
	ar | y;

	using namespace std;
	std::fstream fs("data", ios::out|ios::binary|ios::trunc);
	fs.write(ar.data.data(), ar.data.size());


	Y z;
	pulmotor::archive_vector_in in(ar.data);
	in | z;

	assert(z.px != y.px);
	assert(z.px->x == y.px->y);
}
```



## Serialization Structure

```
 [comment]
 VF - version/flags

<serializable>		:= <primitive>
				 	 | <primitive-array>
					 | VF <struct>
					 | VF <struct-array>
					 | VF <pointer>
				 	 | VF <pointer-array>
				 	 | PLACE(area) <pointer>
				 	 | CONSTRUCT(ptr) <pointer>
				 	 | ALLOC <pointer>
				 	 | PTR <pointer>

<pointer>			:= PTR-ID? <serializable>
<pointer-array>		:= SIZE (PTR-ID? <serializable>){SIZE}
<struct-array>		:= SIZE <struct>{SIZE}
<struct>			:= [--> call custom function] <serializable> *
<primitive-array>	:= SIZE <primitive>{SIZE}
<primitive>			:= arithmetic# | enum#
```

save-load-data	--> save-load if save-load-construct is available otherwise try to empty-construct and do serialize
CONSTRUXT(p)	--> do save-construct; do load-construct and store ptr if non-null; do load-construct and do not expect return value
