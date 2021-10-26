#include <limits>
#include <pulmotor/util.hpp>
#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

#include "base64_nibbleandhalf.h"

int main()
{
	using namespace std::literals;
	pulmotor::romu3 r;

	auto test_enc_dec = [&r](std::string const& desc, size_t SIZE,
		std::function<void(char const*, size_t, char*)> encode,
		std::function<void(char const*, size_t, char*)> decode
	) {

		size_t ENCODED_SIZE = pulmotor::util::base64_encode_length(SIZE);
		size_t DECODED_SIZE = pulmotor::util::base64_decode_length_approx(ENCODED_SIZE);
		char* content = new char[SIZE];
		char* encoded = new char[ENCODED_SIZE];
		char* decoded = new char[DECODED_SIZE];
		PULMOTOR_SCOPE_EXIT(&) { delete[] content; delete[] encoded; delete[] decoded; };

		std::generate_n(content, SIZE, [&r]() { return r.r(256); });

		ankerl::nanobench::Bench().minEpochIterations(16).run("encode "s + desc, [&] {
			encode(content, SIZE, encoded);
			ankerl::nanobench::doNotOptimizeAway(encoded);
		});

		ankerl::nanobench::Bench().minEpochIterations(16).run("decode "s + desc, [&] {
			decode(encoded, SIZE, decoded);
			ankerl::nanobench::doNotOptimizeAway(decoded);
		});

	};


	auto do_test = [&test_enc_dec] (std::string const& lib,
			std::function<void(char const*, size_t, char*)> enc,
			std::function<void(char const*, size_t, char*)> dec)
	{
		test_enc_dec(lib + " base64  16B", 16, enc, dec);
		test_enc_dec(lib + " base64 127B", 127, enc, dec);
		test_enc_dec(lib + " base64   8K", 1024 * 8, enc, dec);
		test_enc_dec(lib + " base64   1M", 1024 * 1024, enc, dec);
	};

	{
		// PULMOTOR
		auto enc = [] (char const* content, size_t SIZE, char* encoded) {
			pulmotor::util::base64_encode(content, SIZE, encoded);
		};
		auto dec = [] (char const* encoded, size_t SIZE, char* decoded) {
			pulmotor::util::base64_decode(encoded, SIZE, decoded);
		};

		do_test("pulmotor", enc, dec);
	}

	{
		// NIBBLE AND HALF
		auto enc = [] (char const* content, size_t SIZE, char* encoded) {
			std::string s = base64(content, SIZE);
			//memcpy(encoded, s.data(), s.size());
		};
		auto dec = [] (char const* encoded, size_t SIZE, char* decoded) {
			std::string s = unbase64(encoded, SIZE);
			//memcpy(decoded, s.data(), s.size());
		};

		do_test("nibbleandhalf", enc, dec);
	}

}