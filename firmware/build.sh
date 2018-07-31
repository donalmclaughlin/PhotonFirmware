mkdir temp_build

rm tcp_photographer.bin
rm temp_build/*.*
cp tcp_photographer/*.* temp_build

cp libraries2/*.h temp_build
cp libraries2/*.cpp temp_build
cp libraries2/*.c temp_build

particle compile photon temp_build --saveTo tcp_photographer.bin
