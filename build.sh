echo "Configuring and building Thirdparty/Seq-CALC ..."

cd Thirdparty/Seq-CALC
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j


echo "   "
echo "Configuring and building this Stereo SLAM Sytem ..."

cd ../../../
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j
