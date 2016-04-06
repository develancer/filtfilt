#define _FILE_OFFSET_BITS 64

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <list>
#include <err.h>
#include <errno.h>
#include <strings.h>

template<typename T>
void array_from_list(T*& array, int& size, const std::list<double>& list) {
	size = list.size();
	array = new T[size];
	std::copy(list.begin(), list.end(), array);
}

double double_from_string(char* string) {
	errno = 0;
	char* end = string;
	double value = strtod(string, &end);
	if (errno || end == string) {
		errx(1, "invalid coefficient '%s'", string);
	}
	return value;
}

int int_from_string(char* string) {
	errno = 0;
	char* end = string;
	int value = strtol(string, &end, 10);
	if (errno || end == string) {
		errx(1, "invalid number '%s'", string);
	}
	return value;
}

int* sequence(int length) {
	int* result = new int[length];
	for (int i=0; i<length; ++i) {
		result[i] = i;
	}
	return result;
}

class Filter {
public:
	virtual void rfilter(FILE* input, FILE* output, unsigned channel_count, double* const a, int na, double* const b, int nb, int* const c, int nc) =0;
};

template<typename T>
class FilterImpl : public Filter {
public:
	void rfilter(FILE* input, FILE* output, unsigned channel_count, double* const a, int na, double* const b, int nb, int* const c, int nc) {
		T buffer[channel_count];
		T input_samples[nb][nc];
		T output_samples[na][nc];
		int input_index = 0, output_index = 0;

		fseeko(input, 0, SEEK_END);
		const off_t sample_size = sizeof(T) * channel_count;
		const off_t byte_count = ftello(input);
		if (byte_count < 0) err(1, "could not calculate file length");
		off_t sample_count = ftello(input) / sample_size;
		while (sample_count-- > 0) {
			if (fseeko(input, -sample_size, SEEK_CUR) < 0) err(1, "could not move in input file");
			if (fread(buffer, sizeof(T), channel_count, input) != channel_count) err(1, "could not read from input file");
			if (fseeko(input, -sample_size, SEEK_CUR) < 0) err(1, "could not move in input file");
			for (int i=0; i<nc; ++i) {
				input_samples[input_index][i] = buffer[c[i]];
				double value = 0.0;
				for (int r=0; r<nb; ++r) {
					int prev_index = input_index - r;
					if (prev_index < 0) prev_index += nb;
					value += b[r] * input_samples[prev_index][i];
				}
				for (int r=1; r<na; ++r) {
					int prev_index = output_index - r;
					if (prev_index < 0) prev_index += na;
					value -= a[r] * output_samples[prev_index][i];
				}
				output_samples[output_index][i] = value / a[0];
			}
			if (fwrite(output_samples[output_index], sizeof(T), nc, output) != static_cast<size_t>(nc)) err(1, "could not write to output file");
			++input_index %= nb;
			++output_index %= na;
		}
	}
};

int main(int argc, char** argv) {
	if (argc < 5) {
		fprintf(stderr,
		"USAGE: %s input_path output_path format channel_count [ selected channels ... ] a [ AR coefficients ... ] b [ FIR coefficients ... ]\n"
		"\tinput_path: path to input file\n"
		"\toutput_path: path to output file, will be overwritten\n"
		"\tformat: data format, currently: float32 or float64\n"
		"\tchannel_count: number of channels in the input file\n"
		"\tselected_channels: selected channels (1..channel_count);\n"
		"\t\tleave empty for filtering all channels\n"
		"\tAR coefficients: coefficients a[1] a[2] ... (a[0] is assumed as 1.0)\n"
		"\tFIR coefficients: coefficients b[0] b[1] b[2] ...\n\n"
		"Program processes the input signal using linear filter:\n"
		"\ty[t] = b[0] x[t] + b[1] x[t-1] + ... - a[1] y[t-1] - a[2] y[t-2] - ...\n"
		"and the filtering is done twice: backward and forward, to eliminate phase shift.\n\n"
		"Current version of the source code is available at\n"
		"\thttps://github.com/develancer/filtfilt\n"
		, argv[0]);
		exit(1);
	}

	std::list<double> la, lb, lc;
	char* input_path = *++argv;
	char* output_path = *++argv;
	char* format = *++argv;
	int channel_count = int_from_string(*++argv);

	la.push_back(1.0);
	std::list<double>* list = 0;
	while (char* arg = *++argv) {
		if (arg[0] && !arg[1]) switch (arg[0]) {
			case 'a': list=&la; continue;
			case 'b': list=&lb; continue;
		}
		if (list) {
			list->push_back(double_from_string(arg));
		} else {
			lc.push_back(int_from_string(arg));
		}
	}

	Filter* filter = 0;
	if (!strcasecmp(format, "float32")) filter = new FilterImpl<float>(); else
	if (!strcasecmp(format, "float64")) filter = new FilterImpl<double>(); else
	errx(1, "unsupported data type: %s\n", format);

	double *a, *b;
	int *c1, *c2;
	int na, nb, nc;
	array_from_list(a, na, la);
	array_from_list(b, nb, lb);
	if (!nb) errx(1, "at least one FIR coefficient must be provided");
	if (lc.empty()) {
		c1 = sequence(nc = channel_count);
		c2 = c1;
	} else {
		array_from_list(c1, nc, lc);
		for (int i=0; i<nc; ++i) {
			if (c1[i] <= 0 || c1[i] > channel_count) {
				errx(1, "invalid channel: %d", c1[i]);
			}
			--c1[i];
		}
		c2 = sequence(nc);
	}

	FILE* input = fopen(input_path, "r");
	if (!input) err(1, "could not open input file '%s'", input_path);
	FILE* temp = tmpfile();
	if (!temp) err(1, "could not create temporary file");
	FILE* output = fopen(output_path, "w");
	if (!output) err(1, "could not create output file '%s'", output_path);

	filter->rfilter(input, temp, channel_count, a, na, b, nb, c1, nc);
	fflush(temp);

	filter->rfilter(temp, output, nc, a, na, b, nb, c2, nc);
	fflush(output);
}
