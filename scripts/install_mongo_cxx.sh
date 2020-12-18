cd ../mongo-cxx-driver
git checkout r3.6.2
cd build
cmake ..                                \
    -DCMAKE_BUILD_TYPE=Release          \
    -DCMAKE_INSTALL_PREFIX=/usr/local
sudo cmake --build . --target EP_mnmlstc_core
cmake --build .
sudo cmake --build . --target install