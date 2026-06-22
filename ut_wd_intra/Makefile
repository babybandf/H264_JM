CXX ?= g++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -Werror
DUMP ?= ../dump_output_cases/akiyo/intra_dump_postorder.bin

.ONESHELL:

.PHONY: h264-reader-check h264-reader-smoke clean

h264-reader-check:
	$(CXX) $(CXXFLAGS) -c H264IntraDumpReader.cc -o /tmp/H264IntraDumpReader.o

h264-reader-smoke:
	$(CXX) $(CXXFLAGS) -I . H264IntraDumpReader.cc -x c++ -o /tmp/h264_dump_smoke - <<-'EOF'
	#include "H264IntraDumpReader.h"
	#include <cstdio>
	int main(int argc, char** argv) {
	  const char* path = (argc > 1) ? argv[1] : "$(DUMP)";
	  H264IntraDump::Reader reader(path);
	  if (!reader.isOpen()) return 2;
	  H264IntraDump::Event ev;
	  int mb = 0, block = 0, pred = 0, metric = 0;
	  while (reader.next(ev)) {
	    if (ev.kind == H264IntraDump::EventKind::MbBegin) mb++;
	    else if (ev.kind == H264IntraDump::EventKind::BlockBegin) block++;
	    else if (ev.kind == H264IntraDump::EventKind::PredPels) pred++;
	    else if (ev.kind == H264IntraDump::EventKind::ModeMetric_) metric++;
	  }
	  std::printf("mb=%d block=%d pred=%d metric=%d\n", mb, block, pred, metric);
	  return (mb > 0 && block > 0 && pred > 0 && pred == metric) ? 0 : 1;
	}
	EOF
	/tmp/h264_dump_smoke "$(DUMP)"

clean:
	rm -f /tmp/H264IntraDumpReader.o /tmp/h264_dump_smoke