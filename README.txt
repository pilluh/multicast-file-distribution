Create a dummy example file (random.data):
bash createrandomfile.bash

Compile:
make CONF=Release

Prepare file metadata:
./dist/Release/GNU-Linux/multicastfiledistribution fprepare random.data /tmp/mltcastdst 65536

Start transmitting file blocks (previously prepared):
./dist/Release/GNU-Linux/multicastfiledistribution ftransmit random.data /tmp/mltcastdst 226.1.1.1 10.0.2.15 4321

Start receiving file blocks (output filename and output directory must be different from the previous ones if running on the same filesystem):
./dist/Release/GNU-Linux/multicastfiledistribution freceive random2.data /tmp/mltcastdst2 226.1.1.1 10.0.2.15 4321

Data blocks and index are available here by default: /tmp/mltcastdst

Check result file is the same as the input file:
md5sum random.data random2.data
