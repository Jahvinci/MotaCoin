#!/bin/bash
echo ""
echo ""
echo "###############################"
echo "#          Compiling          #"		
echo "###############################"
echo ""
echo "Now we begin the compilation....."
echo ""
cd /home/$USER/MotaCoin/src/leveldb/
sh build_detect_platform build_config.mk ./
cd /home/$USER/MotaCoin/src/
echo "Makefile.."
make -f makefile.unix
cd /home/$USER/MotaCoin/
echo "Qmake..."
qmake
echo "Make.."
make
echo "Your wallet is ready."
