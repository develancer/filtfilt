filtfilt
========

Simple command-line utility to perform IIR (or FIR) filtering on a given
multichannel signal. Main features include:

* works (fast!) on-the-fly, without loading the whole signal into memory
* performs filtering twice: backward and forward, to eliminate phase shift
* allows to select only a subset of input channels

## Compilation

Run „make” or compile the file manually. Only C++ compiler is needed.

## Usage

	filtfilt input_path output_path format channel_count [ selected channels ... ] a [ AR coefficients ... ] b [ FIR coefficients ... ]

* _input_path_ is the full path to input file. The file should consist of
floating point values in the byte order of the current machine (no byte-order
conversion is performed). For multichannel signals, first come the samples for
all channels at t=0, then for all channels at t=Δt, and so forth. In other
words, the signal should be written in column-major order (rows = channels,
columns = samples).

* _output_path_ is the full path to output file. It will be overwritten or
created if it does not exist.

* _format_ is the name of the data format, currently only “float32” or “float64”
are supported.

* _channel_count_ is the number of channels in the input file.

* _selected channels_ are the numbers of selected channels, counted from 1
to _channel_count_. If empty, all channels will be filtered and written to output
file.

* _AR coefficients_ are the coefficients a[1] a[2] ... (a[0] is assumed as 1.0)
of the autoregressive part of the filter.

* _FIR coefficients_ are the coefficients b[0] b[1] b[2] ... of the FIR part
of the filter.

Program processes the input signal using linear filter

	y[t] = b[0] x[t] + b[1] x[t-1] + ... - a[1] y[t-1] - a[2] y[t-2] - ...

(the filtering is done twice: backward and forward, to eliminate phase shift)
and writes the resulting signal to the output file.

## Disclaimer

filtfilt is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2 of the License, or (at your option) any later version.

filtfilt is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
empi (file “LICENCE”); if not, write to the Free Software Foundation, Inc.,
51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Author: Piotr Różański <piotr@develancer.pl>
