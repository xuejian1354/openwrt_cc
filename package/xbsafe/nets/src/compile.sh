#!/bin/sh
export PATH=/gateway/crosstoos-mips/usr/mips-linux/lib:/gateway/crosstoos-mips/usr/lib:/gateway/crosstoos-mips/usr/bin:/gateway/crosstoos-mips/lib:$PATH
export LD_LIBRARY_PATH=/gateway/crosstoos-mips/usr/lib:$LD_LIBRARY_PATH
mips-linux-g++ Common.cpp TThread.cpp XbHttp.cpp duktape.c json.c ping.c tracert.c NetInfoCheck.cpp PhysicalSocket.cpp EpNetIA.cpp Downloader.cpp SpeedTest.cpp wanclient.c Lock.cpp RouterLine.cpp TestEpNetSpeed.cpp Main.cpp -I/gateway/crosstoos-mips/usr/include -lpthread -o speed -Wall