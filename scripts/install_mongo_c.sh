cd ../mongo-c-driver
git checkout 1.17.0  # To build a particular release
python build/calc_release_version.py > VERSION_CURRENT
mkdir cmake-build
cd cmake-build
cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF ..
cmake --build .
sudo cmake --build . --target install